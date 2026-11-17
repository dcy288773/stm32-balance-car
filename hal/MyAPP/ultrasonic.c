#include "ultrasonic.h"

#if (ULTRASONIC_ENABLE != 0)

/*
 * 为什么用 DWT 而不是定时器：
 * TIM3 在这个工程里是 200Hz 控制节拍源，不能拿来当测距秒表。
 * DWT->CYCCNT 是 72MHz 自由计数器，精度 14ns，一个定时器都不占。
 *
 * 为什么要"最近几次取最小"：
 * Control() 每 5ms 跑一次、每次约 1ms（读 DMP 的软件 I2C）。
 * 回波如果正好和它重叠，边沿检测会被推迟，测出来的宽度变长 ——
 * 也就是说误差是单向的，只会让距离偏大。取最小值最接近真值。
 */

#define SAMPLE_COUNT                3U           //保留最近几次读数


/* =========================================================
 * 模块内部状态
 * ========================================================= */

static float    sample_cm[SAMPLE_COUNT];    //最近几次的读数
static uint8_t  sample_ok[SAMPLE_COUNT];    //对应的有效性，0=超时
static uint8_t  sample_idx = 0U;            //环形缓冲写指针
static uint32_t last_trig_tick = 0U;        //上次触发的时刻


/* =========================================================
 * 对外变量
 * ========================================================= */

volatile float   Ultrasonic_Distance = ULTRASONIC_INVALID_CM;
volatile uint8_t Ultrasonic_Valid = 0U;



/*******  内部函数  **********/

//自己把 DWT 打开一次。IIC.c 里也开过，重复开没有副作用；
//但如果 MPU 初始化失败、IIC.c 那条路径没走到，这里能兜住，
//否则 CYCCNT 不动，下面的忙等会死循环
static void ultrasonic_dwt_enable(void)
{
    CoreDebug->DEMCR |= (1UL << 24U);  //使能 DWT
    DWT->CYCCNT = 0U;  //清零计数器
    DWT->CTRL |= 1UL;  //启用 CYCCNT
}

//发一次 12us 触发脉冲
static void ultrasonic_trigger(void)
{
    uint32_t t0;

    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_SET);

    t0 = DWT->CYCCNT;
    while ((DWT->CYCCNT - t0) < (12U * ULTRASONIC_CPU_MHZ))
    {
        //忙等 12us，不能用 HAL_Delay，那是毫秒级
    }

    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_RESET);
}

//测一次。成功返回 1 并写入 out_cm，超时或超量程返回 0
static uint8_t ultrasonic_measure_once(float *out_cm)
{
    uint32_t t_start;    //开始等回波的时刻
    uint32_t t1;         //回波上升沿
    uint32_t t2;         //回波下降沿
    uint32_t timeout;    //本次的超时阈值

    ultrasonic_trigger();

    //等回波变高

    t_start = DWT->CYCCNT;
    timeout = ULTRASONIC_ECHO_START_US * ULTRASONIC_CPU_MHZ;
    while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_RESET)
    {
        if ((DWT->CYCCNT - t_start) > timeout)
        {
            return 0U;   //模块没接、没供电，或者前方完全没有反射物
        }
    }
    t1 = DWT->CYCCNT;

    //等回波变低
    timeout = ULTRASONIC_ECHO_MAX_US * ULTRASONIC_CPU_MHZ;
    while (HAL_GPIO_ReadPin(ULTRASONIC_ECHO_PORT, ULTRASONIC_ECHO_PIN) == GPIO_PIN_SET)
    {
        if ((DWT->CYCCNT - t1) > timeout)
        {
            return 0U;   //超出量程
        }
    }
    t2 = DWT->CYCCNT;

    //宽度(CPU周期) / 72 = 微秒，再乘 0.017 = 厘米
    *out_cm = ((float)(t2 - t1) / (float)ULTRASONIC_CPU_MHZ) * ULTRASONIC_CM_PER_US;


    return 1U;
}


/*******  初始化  **********/

void Ultrasonic_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t i;

    ultrasonic_dwt_enable();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    //TRIG：推挽输出，平时保持低电平
    HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_RESET);

    gpio.Pin = ULTRASONIC_TRIG_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ULTRASONIC_TRIG_PORT, &gpio); 

    //ECHO：普通输入，不开中断。模块输出是 5V，PA2 是 5V 容忍脚，可以直接接
    gpio.Pin = ULTRASONIC_ECHO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ULTRASONIC_ECHO_PORT, &gpio);

    //环形缓冲清空
    for (i = 0U; i < SAMPLE_COUNT; i++)  
    {
        sample_cm[i] = ULTRASONIC_INVALID_CM;
        sample_ok[i] = 0U;
    }
    sample_idx = 0U;
    last_trig_tick = HAL_GetTick();

    Ultrasonic_Distance = ULTRASONIC_INVALID_CM;
    Ultrasonic_Valid = 0U;
}


/*******  主循环里调用  **********/

void Ultrasonic_Update(void)
{
    float   cm;          //这次测到的距离
    float   best;        //最近几次里的最小有效值
    uint8_t i;
    uint8_t any_ok;      //最近几次里有没有有效读数

    //没到节拍就立刻返回，不阻塞，硬件测距模块本身有 60ms 的节拍要求，短了上一次的回波会串进来
    if ((HAL_GetTick() - last_trig_tick) < ULTRASONIC_PERIOD_MS)
    {
        return;
    }
    last_trig_tick = HAL_GetTick();

    //测一次，存进环形缓冲
    if (ultrasonic_measure_once(&cm) != 0U)
    {
        sample_cm[sample_idx] = cm;
        sample_ok[sample_idx] = 1U;
    }
    else
    {
        sample_cm[sample_idx] = ULTRASONIC_INVALID_CM;
        sample_ok[sample_idx] = 0U;
    }

    sample_idx++;
    if (sample_idx >= SAMPLE_COUNT)
    {
        sample_idx = 0U;
    }

    //在最近几次里取有效值的最小者：中断打断只会让读数变大，
    //所以最小值最接近真值
    best = ULTRASONIC_INVALID_CM;  
    any_ok = 0U;
    for (i = 0U; i < SAMPLE_COUNT; i++)
    {
        if (sample_ok[i] != 0U)
        {
            any_ok = 1U;
            if (sample_cm[i] < best)
            {
                best = sample_cm[i];
            }
        }
    }

    if (any_ok != 0U)
    {
        Ultrasonic_Distance = best;
        Ultrasonic_Valid = 1U;
    }
    else
    {
        //连续几次都没回波：要么前方很空，要么模块没接上
        //这种情况判为无效，pid.c 里的防撞不生效，车照常能开
        Ultrasonic_Distance = ULTRASONIC_INVALID_CM;
        Ultrasonic_Valid = 0U;
    }
}

#else

/* ULTRASONIC_ENABLE = 0 时保留空接口，main.c 一行都不用改 */

void Ultrasonic_Init(void)
{
}

void Ultrasonic_Update(void)
{
}

#endif /* ULTRASONIC_ENABLE */
