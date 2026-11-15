#include "bluetooth.h"
#include "usart.h"

/*
 * 蓝牙遥控（USART3 + HC-05/HC-06）
 *
 * 中断只管搬运：收 1 字节 -> 压进环形缓冲 -> 记时间戳 -> 重新挂接收
 * 控制里才解析：Control() 里取出全部待处理指令 -> 更新 rc_xxx 四个标志
 *
 * 为什么不学厂商工程直接在 USART3_IRQHandler 里写 if/else：
 * 1 字节时确实没问题，但一旦后面加帧头、校验、多字节长度，
 * 中断耗时变长且不可控，会拖慢 200Hz 控制节拍。
 */


/* =========================================================
 * 缓冲参数
 * ========================================================= */

//环形缓冲深度。用 % 取模，所以不要求是2的幂。真正的约束只有两条：
//1) 下标是 uint8_t，所以不能超过 255
//2) 留一格判满，实际能存 RB_SIZE-1 个字节
//取 64 是留余量：9600 波特率下 5ms 最多进 5 个字节
#define RB_SIZE                 64U

//一个控制周期最多处理几条指令。这是 Control() 的耗时上限，
//和缓冲深度是两件事，所以单独一个常量，不要复用 RB_SIZE
#define MAX_CMDS_PER_CYCLE      16U


/* =========================================================
 * 模块内部状态
 * ========================================================= */

static volatile uint8_t  rb_buf[RB_SIZE]; //环形缓冲本体
static volatile uint8_t  rb_head = 0U;    //写指针，只在中断里动
static volatile uint8_t  rb_tail = 0U;    //读指针，只在 Control() 里动
static volatile uint8_t  rx_byte = 0U;    //Receive_IT 的落点，一次一个字节
static volatile uint32_t rx_last_tick = 0U; //最后一个字节到达的时刻，失联判断用


/* =========================================================
 * 对外变量
 * ========================================================= */

volatile uint8_t  rc_fore = 0U;  //前进
volatile uint8_t  rc_back = 0U;  //后退
volatile uint8_t  rc_left = 0U;  //左转
volatile uint8_t  rc_right = 0U; //右转

//调试用，可以显示到屏幕上
volatile uint32_t Bluetooth_RxCount = 0U;         //一共收了多少字节
volatile uint8_t  Bluetooth_LastByte = 0U;        //最后一个字节是什么
volatile uint8_t  Bluetooth_InitResult = 0xFFU;   //0=挂接收成功，其他=失败

//串口错误统计。用来区分两种方向相反的故障：
//FE/NE 多 -> 波特率不对、线太长、干扰大
//ORE   多 -> 消费太慢（Control() 被拖住）
volatile uint32_t Bluetooth_ErrorCount = 0U;    //错误回调总次数
volatile uint32_t Bluetooth_LastErrorCode = 0U; //最近一次的 ErrorCode 位组合
volatile uint32_t Bluetooth_OreCount = 0U;      //溢出
volatile uint32_t Bluetooth_FeCount = 0U;       //帧错误
volatile uint32_t Bluetooth_NeCount = 0U;       //噪声
volatile uint32_t Bluetooth_PeCount = 0U;       //校验错误，8N1 下正常恒为0


/*******  初始化  **********/

void Bluetooth_Init(void)
{
    rb_head = 0U;
    rb_tail = 0U;
    Bluetooth_AllStop();
    Bluetooth_RxCount = 0U;
    Bluetooth_LastByte = 0U;
    rx_last_tick = HAL_GetTick();

    //返回值必须查：挂接收失败的话后面一个字节都收不到，
    //而且没有任何现象，查起来要从接线一直查到时钟
    if (HAL_UART_Receive_IT(&huart3, (uint8_t *)&rx_byte, 1U) != HAL_OK)
    {
        Bluetooth_InitResult = 1U;
    }
    else
    {
        Bluetooth_InitResult = 0U;
    }
}


/*******  环形缓冲  **********/

//中断里调用。满了就丢新字节，保证读侧不会被半覆盖
static void rb_push(uint8_t data)
{
    uint8_t next = (uint8_t)((rb_head + 1U) % RB_SIZE);

    if (next != rb_tail)
    {
        rb_buf[rb_head] = data;
        rb_head = next;
    }
}

//Control() 里调用。空了返回 0xFF，因为 0xFF 不是合法指令
static uint8_t rb_pop(void)
{
    uint8_t data;

    if (rb_tail == rb_head)
    {
        return 0xFFU;
    }

    data = rb_buf[rb_tail];
    rb_tail = (uint8_t)((rb_tail + 1U) % RB_SIZE);
    return data;
}


/*******  指令解析  **********/

//一条指令只管一个方向，四个标志互斥
static void rc_apply(uint8_t cmd)
{
    switch (cmd)
    {
        case RC_CMD_FORE:
            rc_fore = 1U; rc_back = 0U; rc_left = 0U; rc_right = 0U;
            break;

        case RC_CMD_BACK:
            rc_fore = 0U; rc_back = 1U; rc_left = 0U; rc_right = 0U;
            break;

        case RC_CMD_LEFT:
            rc_fore = 0U; rc_back = 0U; rc_left = 1U; rc_right = 0U;
            break;

        case RC_CMD_RIGHT:
            rc_fore = 0U; rc_back = 0U; rc_left = 0U; rc_right = 1U;
            break;

        case RC_CMD_STOP:
            rc_fore = 0U; rc_back = 0U; rc_left = 0U; rc_right = 0U;
            break;

        default:
            //未知字节直接丢，不当成"停"，免得误码导致突然停车
            break;
    }
}


/* =========================================================
 * 对外接口
 * ========================================================= */

//Control() 里调用，把缓冲里的指令全取出来
void Bluetooth_UpdateCommand(void)
{
    uint8_t cmd;
    uint8_t guard = 0U; //本周期已处理条数

    while ((cmd = rb_pop()) != 0xFFU)
    {
        rc_apply(cmd);

        //一周期最多处理16条，防止异常流量把 Control() 拖长
        if (++guard >= MAX_CMDS_PER_CYCLE)
        {
            break;
        }
    }
}

//1 = RC_TIMEOUT_MS 内收到过字节。
//默认只用来在屏幕上判断"链路还活着"，不参与控制；
//把 bluetooth.h 里的 RC_STOP_ON_TIMEOUT 改成 1，它才会变成失联停车判据
uint8_t Bluetooth_IsOnline(void)
{
    if ((HAL_GetTick() - rx_last_tick) > (uint32_t)RC_TIMEOUT_MS)
    {
        return 0U;
    }

    return 1U;
}

void Bluetooth_AllStop(void)
{
    rc_fore  = 0U;
    rc_back  = 0U;
    rc_left  = 0U;
    rc_right = 0U;
}


/* =========================================================
 * HAL 回调
 * ========================================================= */

//收到一个字节。这里只搬运，不解析
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        rb_push(rx_byte);

        Bluetooth_RxCount++;  //统计接收字节数
        Bluetooth_LastByte = rx_byte;  //统计最后一个字节
        rx_last_tick = HAL_GetTick();  //统计最后一个字节到达的时刻

        (void)HAL_UART_Receive_IT(&huart3, (uint8_t *)&rx_byte, 1U);  //重新挂接收
    }
}

/*
 * 溢出/噪声错误会让 HAL 中止接收，必须重新挂上，
 * 否则收一两个字节就"死"了，而且现象很迷惑（有时候能用）。
 *
 * 关于清标志：F1 的 stm32f1xx_hal_uart.h 里
 *   __HAL_UART_CLEAR_PEFLAG / FEFLAG / NEFLAG / OREFLAG
 * 这四个宏等价于同一个动作——读一次 SR，再读一次 DR。
 * 参考手册对 PE/FE/NE/ORE 的清除方式就是"读 SR 后读 DR"，
 * 所以调一次 __HAL_UART_CLEAR_OREFLAG，四个标志就都清了。
 *
 * F1 HAL 在 ORE 分支里只调 UART_EndRxTransfer()，自己不清标志，
 * 所以这一句是必需的，不是多余的。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uint32_t ec; //错误码位组合

    if (huart->Instance == USART3)
    {
        //先取错误码再重新挂接收：换个 HAL 版本时 ErrorCode 可能在
        //Start_Receive_IT 里被清掉，所以顺序不能反
        ec = huart->ErrorCode;

        Bluetooth_ErrorCount++;
        Bluetooth_LastErrorCode = ec;

        if ((ec & HAL_UART_ERROR_ORE) != 0U) Bluetooth_OreCount++;  //统计溢出错误
        if ((ec & HAL_UART_ERROR_FE)  != 0U) Bluetooth_FeCount++;  //统计帧错误
        if ((ec & HAL_UART_ERROR_NE)  != 0U) Bluetooth_NeCount++;  //统计噪声错误
        if ((ec & HAL_UART_ERROR_PE)  != 0U) Bluetooth_PeCount++;  //统计校验错误

        __HAL_UART_CLEAR_OREFLAG(huart);  //清除 ORE/FE/NE/PE 标志，避免 HAL 再次调用 ErrorCallback

        (void)HAL_UART_Receive_IT(&huart3, (uint8_t *)&rx_byte, 1U);
    }
}
