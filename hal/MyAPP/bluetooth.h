#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*
 * 蓝牙遥控（USART3 + JDY-31，9600 8N1）
 *
 * 协议沿用厂商手机 App 的 1 字节指令，方便直接用现成 App：
 *   0x00 停   0x01 前   0x05 后   0x07 左   0x03 右
 */


/* =========================================================
 * 协议
 * ========================================================= */

#define RC_CMD_STOP     0x00U  //停
#define RC_CMD_FORE     0x01U  //前
#define RC_CMD_RIGHT    0x03U  //右
#define RC_CMD_BACK     0x05U  //后
#define RC_CMD_LEFT     0x07U  //左


/* =========================================================
 * 参数
 * ========================================================= */

//指令是否在失联后自动清零。
//0 = 指令一直保持（默认，和厂商工程一致）：收到新指令前，上一个方向一直有效，
//    车会一直前进/后退/转向，直到你发"停"或反方向指令。
//1 = 打开失联保护：超过 RC_TIMEOUT_MS 没收到任何字节，方向全部清零、车停下。
//    代价是手机 App 如果只在按键瞬间发一次，车就只会动 RC_TIMEOUT_MS 那么久。
#define RC_STOP_ON_TIMEOUT   0

//失联判定时间。
//RC_STOP_ON_TIMEOUT = 0 时，它只用于更新"在线"标志（给屏幕看）；
//RC_STOP_ON_TIMEOUT = 1 时，它就是停车阈值。
#define RC_TIMEOUT_MS   300U

//前后目标速度上限。单位是"编码器计数 / 10ms"（和 Velocity 的输入同量纲），
//30 大约相当于 0.4 m/s。不要按 m/s 去理解这个数字
#define RC_SPEED_MAX    30.0f

//转向目标上限。这是 Turn_Kp 的输入单位，不是速度
#define RC_TURN_MAX     150.0f

//斜坡步长。Control() 每 5ms 调用一次，所以：
//  0.5  x (200/10) = 每 10ms 变化 1   （前后）
//  15.0 x (200/10) = 每 10ms 变化 30  （转向）
//和厂商工程的 ±1 / ±30 一致，只是换了调用频率
#define RC_SPEED_STEP   0.5f
#define RC_TURN_STEP    15.0f


/* =========================================================
 * 对外变量
 * ========================================================= */

extern volatile uint8_t rc_fore;  //前进，1=按下
extern volatile uint8_t rc_back;  //后退
extern volatile uint8_t rc_left;  //左转
extern volatile uint8_t rc_right; //右转

extern volatile uint32_t Bluetooth_RxCount;      //一共收了多少字节
extern volatile uint8_t  Bluetooth_LastByte;     //最后一个字节
extern volatile uint8_t  Bluetooth_InitResult;   //0=挂接收成功，其他=失败

//串口错误统计，用来判断故障方向
extern volatile uint32_t Bluetooth_ErrorCount;     //错误回调总次数
extern volatile uint32_t Bluetooth_LastErrorCode;  //最近一次的 ErrorCode 位组合
extern volatile uint32_t Bluetooth_OreCount;       //溢出：消费太慢
extern volatile uint32_t Bluetooth_FeCount;        //帧错误：波特率不对/线太长
extern volatile uint32_t Bluetooth_NeCount;        //噪声：干扰
extern volatile uint32_t Bluetooth_PeCount;        //校验错误：8N1 下恒为0


/* =========================================================
 * 对外函数
 * ========================================================= */

void    Bluetooth_Init(void);          //main() 里调用一次
void    Bluetooth_UpdateCommand(void); //Control() 里调用，取出全部待处理指令
uint8_t Bluetooth_IsOnline(void);      //1 = RC_TIMEOUT_MS 内收到过字节（仅作参考）
void    Bluetooth_AllStop(void);       //清空四个方向标志

#ifdef __cplusplus
}
#endif

#endif /* __BLUETOOTH_H__ */
