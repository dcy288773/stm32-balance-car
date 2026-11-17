#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*
 * HC-SR04 超声波测距，只用来防撞，不参与姿态和速度环
 *
 * PA3 给 12us 高电平触发，模块发 8 个 40kHz 脉冲，PA2 变高；
 * PA2 高电平的宽度就是声波往返时间，宽度 / 72 * 0.017 = 厘米。
 *
 * 计时用 DWT->CYCCNT，不占任何定时器 ——
 * TIM3 在这个工程里是 200Hz 控制节拍源，抢不了。
 */


/* =========================================================
 * 总开关
 * ========================================================= */

//改成 0 就完全不启用，pid.c 里的防撞限速跟着失效，车回到没有超声波的状态
#define ULTRASONIC_ENABLE           1


/* =========================================================
 * 引脚
 * ========================================================= */

#define ULTRASONIC_TRIG_PORT        GPIOA        //触发脚端口
#define ULTRASONIC_TRIG_PIN         GPIO_PIN_3   //PA3，推挽输出
#define ULTRASONIC_ECHO_PORT        GPIOA        //回波脚端口
#define ULTRASONIC_ECHO_PIN         GPIO_PIN_2   //PA2，普通输入，不开中断

#define ULTRASONIC_CPU_MHZ          72U          //主频，用来把 CPU 周期换成微秒
#define ULTRASONIC_CM_PER_US        0.017f       //声速 340m/s 往返除 2，单位 cm/us


/* =========================================================
 * 测距参数
 * ========================================================= */

//两次触发的间隔。HC-SR04 要求 >= 60ms，短了上一次的回波会串进来
#define ULTRASONIC_PERIOD_MS        65U

//等回波开始的超时。模块没接、没供电时靠它退出，不然会卡死
#define ULTRASONIC_ECHO_START_US    2000U

//回波宽度上限。6ms 约等于 103cm，超过就认为超出量程
//量程只做到 100cm，是为了缩短阻塞时间、少被 200Hz 中断打断
#define ULTRASONIC_ECHO_MAX_US      6000U

//超量程时的距离值
#define ULTRASONIC_INVALID_CM       999.0f


/* =========================================================
 * 防撞阈值
 * ========================================================= */

#define ULTRASONIC_STOP_CM          20.0f        //小于这个距离：前进目标压到 0
#define ULTRASONIC_RESUME_CM        30.0f        //退到这个距离之外才恢复，10cm 滞回
#define ULTRASONIC_SLOW_CM          60.0f        //从 60cm 开始线性压速度


/* =========================================================
 * 对外变量
 * ========================================================= */

extern volatile float   Ultrasonic_Distance;     //cm，无效时是 ULTRASONIC_INVALID_CM
extern volatile uint8_t Ultrasonic_Valid;        //1 = 这次读数可用



/* =========================================================
 * 对外函数
 * ========================================================= */

void Ultrasonic_Init(void);      //main() 里调用一次
void Ultrasonic_Update(void);    //主循环里反复调用，内部自己控 65ms 节拍

#ifdef __cplusplus
}
#endif

#endif /* __ULTRASONIC_H__ */
