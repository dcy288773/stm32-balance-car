#include "pid.h"

#include "tim.h"

#include "encoder.h"
#include "motor.h"
#include "bluetooth.h"
#include "ultrasonic.h"

#include "mpu6050.h"
#include "inv_mpu.h"

/*
 * 直立环 200Hz：每个 DMP 新样本跑一次
 * 速度环  50Hz：每 4 个样本跑一次
 *
 * 速度环输入已折算回原来的"每 10ms 编码器计数"单位，所以标准库那套
 * 调好的增益可以原样沿用，物理意义不变。
 */

#define BALANCE_LOOP_HZ             200U  //直立环频率
#define SPEED_LOOP_HZ                50U  //速度环频率
#define SPEED_LOOP_DIVIDER           (BALANCE_LOOP_HZ / SPEED_LOOP_HZ)  //速度环分频计数

#define SPEED_COUNT_TO_10MS          0.5f  //4个5ms样本=20ms，除以2折算回原10ms单位

#define SPEED_LPF_ALPHA_100HZ        0.7f  //原滤波系数是按100Hz定的；50Hz外环每次
                                          //做两个虚拟10ms子步，比直接改系数更接近
                                          //原来的离散动态
#define SPEED_INTEGRAL_LIMIT         20000.0f  //速度环积分限幅

#define MOTOR_PWM_LIMIT              7200  //电机PWM限幅，必须等于TIM1的ARR


/* =========================================================
 * 姿态数据
 * ========================================================= */

float Pitch = 0.0f;  //横滚角，直立环用这个
float Roll  = 0.0f;  //俯仰角
float Yaw   = 0.0f;  //偏航角

short gyrox = 0;  //X轴角速度
short gyroy = 0;  //Y轴角速度，直立环微分用这个
short gyroz = 0;  //Z轴角速度，转向环用这个


/* =========================================================
 * 编码器数据
 * ========================================================= */

int Encoder_Left  = 0;  //单个5ms窗口的原始计数
int Encoder_Right = 0;

float Encoder_Left_10ms  = 0.0f;  //20ms累计折算成10ms单位，速度环的输入
float Encoder_Right_10ms = 0.0f;


/* =========================================================
 * 电机输出
 * ========================================================= */

int MOTO1 = 0;  //最终输出，限幅后的
int MOTO2 = 0;


/* =========================================================
 * 控制参数（标准库上验证过的值）
 * ========================================================= */

float Med_Angle = 0.0f;      //机械中值，需要自己标定
float Target_Speed = 0.0f;   //速度环目标，蓝牙遥控改这个
float Turn_Speed = 0.0f;     //转向环目标，蓝牙遥控改这个

float Vertical_Kp = -400.0f;   //直立环 P
float Vertical_Kd = -1.92f;    //直立环 D

float Velocity_Kp = -0.44f;    //速度环 P
float Velocity_Ki = -0.0022f;  //速度环 I

float Turn_Kd = 0.6f;   //转向环 D：不打方向时把车头拉直，也就是直线约束
float Turn_Kp = 20.0f;  //转向环 P：把遥控转向量换成差速

//平衡/转向输出保持整型（PWM域）；速度输出保持float，否则目标角度会按整度跳
int Vertical_out = 0;
float Velocity_out = 0.0f;
int Turn_out = 0;

//运行诊断量，不参与控制，用来确认200Hz有没有跟上
volatile uint32_t Control_Count = 0U;              //控制总次数
volatile uint32_t Control_DMP_ErrorCount = 0U;     //DMP错误次数
volatile uint32_t Control_DMP_CatchupPackets = 0U; //没跟上被丢弃的包数
volatile uint32_t Control_DMP_BacklogResets = 0U;  //FIFO积压清空次数
volatile uint32_t Control_MaxCycles = 0U;          //单次控制耗时峰值（CPU周期）
volatile uint32_t Control_OverrunCount = 0U;       //超时次数，一次控制超过5ms记一次

//限幅前的原始输出就已经超限的标志，速度环用它做抗积分饱和
static uint8_t velocity_motor_saturated = 0U;

static float abs_float(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float clamp_float(float value, float low, float high)
{
    if (value > high) return high;
    if (value < low)  return low;
    return value;
}


/*******  直立环  200Hz  5ms  **********/
//Med:目标角度  Angle:实际角度  gyro_Y:角速度

int Vertical(float Med,
             float Angle,
             float gyro_Y)
{
    float pwm_out;

    pwm_out =
        Vertical_Kp * (Angle - Med)
        +
        Vertical_Kd * gyro_Y;

    return (int)pwm_out;
}


/*******  速度环  50Hz  20ms  **********/

float Velocity(float Target,
               float encoder_left,
               float encoder_right)
{
    static float Encoder_S = 0.0f;            //积分量
    static float EnC_Err_Lowout_last = 0.0f;  //上一次的滤波输出

    float Encoder_Err;         //速度误差
    float EnC_Err_Lowout_mid;  //第一级滤波，也参与积分
    float EnC_Err_Lowout;      //第二级滤波，参与P
    float integral_candidate;  //积分候选值
    float pwm_out;             //速度环输出

    Encoder_Err =
        (encoder_left + encoder_right)
        -
        Target;

    //两级一阶低通，等效原来100Hz下的两拍
    EnC_Err_Lowout_mid =
        (1.0f - SPEED_LPF_ALPHA_100HZ) * Encoder_Err
        +
        SPEED_LPF_ALPHA_100HZ * EnC_Err_Lowout_last;

    EnC_Err_Lowout =
        (1.0f - SPEED_LPF_ALPHA_100HZ) * Encoder_Err
        +
        SPEED_LPF_ALPHA_100HZ * EnC_Err_Lowout_mid;

    EnC_Err_Lowout_last = EnC_Err_Lowout;

    //原100Hz控制器把两个10ms的滤波值都积了进去，这里同样加两项，
    //这样Ki的时间强度和原来一致
    integral_candidate =
        Encoder_S
        +
        EnC_Err_Lowout_mid
        +
        EnC_Err_Lowout;

    //积分限幅
    integral_candidate =
        clamp_float(
            integral_candidate,
            -SPEED_INTEGRAL_LIMIT,
            SPEED_INTEGRAL_LIMIT
        );

    //抗积分饱和：正常就接受新积分；电机已经饱和时，
    //只接受让积分绝对值变小的那次更新
    if (velocity_motor_saturated == 0U)
    {
        Encoder_S = integral_candidate;
    }
    else if (abs_float(integral_candidate) < abs_float(Encoder_S))
    {
        Encoder_S = integral_candidate;
    }

    //饱和标志管整个20ms窗口，用掉就清零，开始新窗口
    velocity_motor_saturated = 0U;

    //急停、跌倒、遥控失联后清积分，免得残留把车窜一下
    if (stop == 1)
    {
        Encoder_S = 0.0f;
        stop = 0;
    }

    pwm_out =
        Velocity_Kp * EnC_Err_Lowout
        +
        Velocity_Ki * Encoder_S;

    return pwm_out;
}


/*******  转向环  200Hz  5ms  **********/
//gyro_Z:Z轴角速度  RC:遥控转向量

int Turn(int gyro_Z,
         int RC)
{
    float pwm_out;

    pwm_out =
        Turn_Kd * gyro_Z
        +
        Turn_Kp * RC;

    return (int)pwm_out;
}


/*******  小车控制  200Hz  **********/
//由 TIM3 定时中断调用，每 5ms 一次

void Control(void)
{
    static uint8_t speed_divider = 0U;          //速度环分频计数
    static int32_t encoder_left_accum = 0;      //左编码器累计
    static int32_t encoder_right_accum = 0;     //右编码器累计
    static uint8_t dmp_consecutive_errors = 0U; //DMP连续错误计数

    uint32_t cycle_start = DWT->CYCCNT;  //本轮开始的CPU周期
    uint32_t elapsed;                    //本轮耗时
    uint8_t dmp_result;                  //DMP返回码
    uint8_t discarded_packets = 0U;      //被丢弃的FIFO包数

    int PWM_out;    //直立环输出
    int raw_moto1;  //限幅前的原始输出
    int raw_moto2;

    //一次FIFO读取同时给出姿态角和校准后的角速度，保证直立环的P和D
    //用的是同一个MPU样本。积压太深时函数会清FIFO并返回错误，
    //避免拿旧姿态去控车。
    //
    //注意形参顺序是 (pitch, roll, yaw, gx, gy, gz)：
    //这里传的是 &Roll,&Pitch,&Yaw,&gyroy,&gyrox,&gyroz，
    //所以 Roll 里装的是俯仰角、Pitch 里装的是横滚角、gyroy 里装的是X轴角速度。
    //直立环用的 Pitch 和 gyroy 恰好是同一个轴，是对的。
    dmp_result =
        mpu_dmp_get_data_sync(
            &Roll, &Pitch, &Yaw,
            &gyroy, &gyrox, &gyroz,
            &discarded_packets
        );

    //DMP读取失败
    if (dmp_result != 0U)
    {
        Control_DMP_ErrorCount++;
        if (dmp_result == 4U)  //积压太多，程序主动清空了FIFO
        {
            Control_DMP_BacklogResets++;
        }

        if (dmp_consecutive_errors < 255U)
            dmp_consecutive_errors++;

        //丢掉这个异常的速度窗口，别让脏数据在之后变成一个速度尖峰
        (void)Read_Speed(&htim2);
        (void)Read_Speed(&htim4);
        encoder_left_accum = 0;
        encoder_right_accum = 0;
        speed_divider = 0U;

        //坏一帧：最多5ms保持上一次PWM；连续坏两帧：姿态源不可信，急停
        if (dmp_consecutive_errors >= 2U)
        {
            MOTO1 = 0;
            MOTO2 = 0;
            Velocity_out = 0.0f;
            Load(0, 0);
            stop = 1;
        }

        elapsed = (uint32_t)(DWT->CYCCNT - cycle_start);
        if (elapsed > Control_MaxCycles)
            Control_MaxCycles = elapsed;
        if (elapsed > (SystemCoreClock / BALANCE_LOOP_HZ))
            Control_OverrunCount++;
        return;
    }

    dmp_consecutive_errors = 0U;

    //没跟上DMP，有包被丢弃
    if (discarded_packets != 0U)
    {
        Control_DMP_CatchupPackets += discarded_packets;

        //编码器计数已经跨过不止一个5ms周期，直接重开测量窗口，
        //不能把这个更长的计数当成一个普通样本
        (void)Read_Speed(&htim2);
        (void)Read_Speed(&htim4);
        encoder_left_accum = 0;
        encoder_right_accum = 0;
        speed_divider = 0U;
    }
    else
    {
        //实测极性：左 = -TIM2，右 = +TIM4
        Encoder_Left  = -Read_Speed(&htim2);
        Encoder_Right =  Read_Speed(&htim4);

        encoder_left_accum  += Encoder_Left;
        encoder_right_accum += Encoder_Right;

        //蓝牙遥控：只改目标，绝不直接改PWM。
        //指令默认一直保持：收到新指令前，上一个方向一直有效（和厂商工程一致）。
        //要停车就发"停"指令 0x00，或者发反方向指令。
        Bluetooth_UpdateCommand();

#if (RC_STOP_ON_TIMEOUT != 0)
        //只有打开了失联保护，断连才清零；默认关闭，指令一直保持
        if (Bluetooth_IsOnline() == 0U)
        {
            Bluetooth_AllStop();
            stop = 1;
        }
#endif

        //前后：每5ms加0.5，等效每10ms加1，一直加到 RC_SPEED_MAX 就保持
        if (rc_fore && !rc_back)
        {
            Target_Speed += RC_SPEED_STEP;
        }
        else if (rc_back && !rc_fore)
        {
            Target_Speed -= RC_SPEED_STEP;
        }
        else
        {
            //收到停（或两边都按）：斜坡回零，不是瞬间归零。
            //从满速 30 一步跳到 0，速度环误差会突然变成 30，输出瞬间拽十几度，
            //车会狠狠点一下头。斜坡回零只要 300ms，感觉就是"刹住"
            if (Target_Speed > RC_SPEED_STEP)
            {
                Target_Speed -= RC_SPEED_STEP;
            }
            else if (Target_Speed < -RC_SPEED_STEP)
            {
                Target_Speed += RC_SPEED_STEP;
            }
            else
            {
                Target_Speed = 0.0f;
            }
        }

        //左右：每5ms加15，等效每10ms加30，一直加到 RC_TURN_MAX 就保持
        if (rc_left && !rc_right)
        {
            Turn_Speed -= RC_TURN_STEP;
        }
        else if (rc_right && !rc_left)
        {
            Turn_Speed += RC_TURN_STEP;
        }
        else
        {
            //同样斜坡回零。Turn_Kp*150 = 3000 个PWM，瞬间归零会甩一下车头
            if (Turn_Speed > RC_TURN_STEP)
            {
                Turn_Speed -= RC_TURN_STEP;
            }
            else if (Turn_Speed < -RC_TURN_STEP)
            {
                Turn_Speed += RC_TURN_STEP;
            }
            else
            {
                Turn_Speed = 0.0f;
            }
        }

        //打方向时去掉直线约束，否则 Turn_Kd*gyroz 会把车头顶回来，
        //表现为转得很肉、松手往回弹
        Turn_Kd = (rc_left || rc_right) ? 0.0f : 0.6f;

        //目标限幅
        Target_Speed = Target_Speed > RC_SPEED_MAX ? RC_SPEED_MAX
                     : (Target_Speed < -RC_SPEED_MAX ? -RC_SPEED_MAX : Target_Speed);

        Turn_Speed = Turn_Speed > RC_TURN_MAX ? RC_TURN_MAX
                   : (Turn_Speed < -RC_TURN_MAX ? -RC_TURN_MAX : Turn_Speed);

#if (ULTRASONIC_ENABLE != 0)
        /* 防撞：只压前进方向的速度上限。
         * 不碰 PWM、不碰 PID 增益；倒车（负值）和转向完全不受影响。
         * 用 blocked 做滞回：20cm 刹停后，要退到 30cm 才恢复，
         * 否则车会在临界距离上走走停停地抖。 */
        if (Ultrasonic_Valid != 0U)
        {
            static uint8_t blocked = 0U;
            float dist = Ultrasonic_Distance;
            float limit;

            if (blocked != 0U)
            {
                if (dist > ULTRASONIC_RESUME_CM)
                {
                    blocked = 0U;
                }
            }
            else
            {
                if (dist < ULTRASONIC_STOP_CM)
                {
                    blocked = 1U;
                }
            }

            if (blocked != 0U)
            {
                limit = 0.0f;
            }
            else if (dist < ULTRASONIC_SLOW_CM)
            {
                limit = RC_SPEED_MAX
                      * (dist - ULTRASONIC_STOP_CM)
                      / (ULTRASONIC_SLOW_CM - ULTRASONIC_STOP_CM);
            }
            else
            {
                limit = RC_SPEED_MAX;
            }

            if (Target_Speed > limit)
            {
                Target_Speed = limit;
            }
        }
#endif

        //速度环50Hz：累计4个5ms编码器窗口，再折算回10ms单位
        speed_divider++;
        if (speed_divider >= SPEED_LOOP_DIVIDER)
        {
            speed_divider = 0U;

            Encoder_Left_10ms =
                (float)encoder_left_accum * SPEED_COUNT_TO_10MS;

            Encoder_Right_10ms =
                (float)encoder_right_accum * SPEED_COUNT_TO_10MS;

            encoder_left_accum = 0;
            encoder_right_accum = 0;

            Velocity_out =
                Velocity(
                    Target_Speed,
                    Encoder_Left_10ms,
                    Encoder_Right_10ms
                );
        }
    }

    //直立环每个200Hz新样本都跑一次。
    //加上 Velocity_out 就是在动态修改直立环的目标角度，小车靠它前进后退
    Vertical_out =
        Vertical(
            Velocity_out + Med_Angle,
            Pitch,
            gyroy
        );

    Turn_out =
        Turn(
            gyroz,
            (int)Turn_Speed
        );

    PWM_out = Vertical_out;

    //转向差速：左轮减、右轮加
    MOTO1 = PWM_out - Turn_out;
    MOTO2 = PWM_out + Turn_out;

    //跌倒保护必须写在 Load() 之前。旧代码先 Load 再 Stop，
    //在已经越过跌倒角时每周期还会打出一个高PWM脉冲。
    if (GFP_abs((int)(Pitch - Med_Angle)) > 60)
    {
        MOTO1 = 0;
        MOTO2 = 0;
        Velocity_out = 0.0f;
        Load(0, 0);
        stop = 1;

        encoder_left_accum = 0;
        encoder_right_accum = 0;
        speed_divider = 0U;
        (void)Read_Speed(&htim2);
        (void)Read_Speed(&htim4);

        Control_Count++;
        elapsed = (uint32_t)(DWT->CYCCNT - cycle_start);
        if (elapsed > Control_MaxCycles)
            Control_MaxCycles = elapsed;
        if (elapsed > (SystemCoreClock / BALANCE_LOOP_HZ))
            Control_OverrunCount++;
        return;
    }

    //饱和判断必须在 Limit() 之前，否则已经被限幅过就永远判断不出超限
    raw_moto1 = MOTO1;
    raw_moto2 = MOTO2;

    if ((raw_moto1 > MOTOR_PWM_LIMIT) ||
        (raw_moto1 < -MOTOR_PWM_LIMIT) ||
        (raw_moto2 > MOTOR_PWM_LIMIT) ||
        (raw_moto2 < -MOTOR_PWM_LIMIT))
    {
        velocity_motor_saturated = 1U;
    }

    Limit(&MOTO1, &MOTO2);
    Load(MOTO1, MOTO2);

    Control_Count++;

    elapsed = (uint32_t)(DWT->CYCCNT - cycle_start);
    if (elapsed > Control_MaxCycles)
        Control_MaxCycles = elapsed;
    if (elapsed > (SystemCoreClock / BALANCE_LOOP_HZ))
        Control_OverrunCount++;
}
