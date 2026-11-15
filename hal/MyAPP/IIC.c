#include "IIC.h"


/*
 * =========================================================
 * MPU6050 Software I2C
 *
 * SCL -> PB4
 * SDA -> PB3
 *
 * 这里按成功标准库 mpuiic.c 的行为实现
 * =========================================================
 */


/* ---------------- GPIO操作 ---------------- */

#define IIC_SCL_H()     (GPIOB->BSRR = GPIO_PIN_4)
#define IIC_SCL_L()     (GPIOB->BRR  = GPIO_PIN_4)

#define IIC_SDA_H()     (GPIOB->BSRR = GPIO_PIN_3)
#define IIC_SDA_L()     (GPIOB->BRR  = GPIO_PIN_3)

#define IIC_READ_SDA()   ((GPIOB->IDR & GPIO_PIN_3) != 0)


/*
 * PB3：
 *
 * 输出：
 * MODE = 11 -> 50MHz
 * CNF  = 00 -> Push-Pull
 *
 * 即 CRL nibble = 0x3
 */
static void IIC_SDA_OUT(void)
{
    GPIOB->CRL &= ~(0xFUL << 12);
    GPIOB->CRL |=  (0x3UL << 12);
}


/*
 * PB3：
 *
 * 输入上拉：
 * MODE = 00
 * CNF  = 10
 *
 * CRL nibble = 0x8
 */
static void IIC_SDA_IN(void)
{
    GPIOB->CRL &= ~(0xFUL << 12);
    GPIOB->CRL |=  (0x8UL << 12);

    /*
     * 输入上拉：
     * ODR对应位必须为1
     */
    IIC_SDA_H();
}


/* =========================================================
 * DWT初始化
 *
 * 用CPU周期计数器实现精确忙等待。
 *
 * 这样即使在EXTI中断里面延时，
 * 也不依赖HAL SysTick。
 * ========================================================= */

static void IIC_DWT_Init(void)
{
    CoreDebug->DEMCR |= (1UL << 24);

    DWT->CYCCNT = 0;

    DWT->CTRL |= 1UL;
}


/* =========================================================
 * 2us延时
 *
 * 成功标准库：
 *
 * MPU_IIC_Delay()
 * {
 *     delay_us(2);
 * }
 * ========================================================= */

static void IIC_Delay(void)
{
    uint32_t start;
    uint32_t ticks;

    ticks =
        SystemCoreClock / 500000UL;     /* 2us */

    start =
        DWT->CYCCNT;

    while((uint32_t)(DWT->CYCCNT - start) < ticks)
    {
    }
}


/* =========================================================
 * DMP使用的毫秒延时
 *
 * 不再使用 HAL_Delay。
 * ========================================================= */

void IIC_DelayMs(uint32_t ms)
{
    uint32_t start;
    uint32_t ticks;

    while(ms--)
    {
        ticks =
            SystemCoreClock / 1000UL;

        start =
            DWT->CYCCNT;

        while((uint32_t)(DWT->CYCCNT - start) < ticks)
        {
        }
    }
}


/* =========================================================
 * GPIO初始化
 *
 * 成功标准库初始化PB3/PB4为50MHz Push-Pull。
 *
 * SDA读取时再动态切输入。
 * ========================================================= */

void IIC_GPIO_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();


    IIC_DWT_Init();


    /*
     * PB3
     */
    GPIOB->CRL &= ~(0xFUL << 12);

    GPIOB->CRL |=
        (0x3UL << 12);


    /*
     * PB4
     */
    GPIOB->CRL &= ~(0xFUL << 16);

    GPIOB->CRL |=
        (0x3UL << 16);


    IIC_SDA_H();

    IIC_SCL_H();
}


/* =========================================================
 * START
 * ========================================================= */

void IIC_Start(void)
{
    IIC_SDA_OUT();


    IIC_SDA_H();

    IIC_SCL_H();

    IIC_Delay();


    IIC_SDA_L();

    IIC_Delay();


    IIC_SCL_L();
}


/* =========================================================
 * STOP
 * ========================================================= */

void IIC_Stop(void)
{
    IIC_SDA_OUT();


    IIC_SCL_L();

    IIC_SDA_L();


    IIC_Delay();


    IIC_SCL_H();

    IIC_SDA_H();


    IIC_Delay();
}


/* =========================================================
 * 等ACK
 *
 * 这里和你之前IIC.c区别很大。
 *
 * 成功标准库不是读取一次SDA就结束，
 * 而是会等从机真正拉低SDA。
 * ========================================================= */

uint8_t IIC_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;


    IIC_SDA_IN();


    IIC_SDA_H();

    IIC_Delay();


    IIC_SCL_H();

    IIC_Delay();


    while(IIC_READ_SDA())
    {
        ucErrTime++;


        if(ucErrTime > 250)
        {
            IIC_Stop();

            return 1;
        }
    }


    IIC_SCL_L();


    return 0;
}


/* =========================================================
 * ACK
 * ========================================================= */

void IIC_Ack(void)
{
    IIC_SCL_L();


    IIC_SDA_OUT();

    IIC_SDA_L();


    IIC_Delay();


    IIC_SCL_H();

    IIC_Delay();


    IIC_SCL_L();
}


/* =========================================================
 * NACK
 * ========================================================= */

void IIC_NAck(void)
{
    IIC_SCL_L();


    IIC_SDA_OUT();

    IIC_SDA_H();


    IIC_Delay();


    IIC_SCL_H();

    IIC_Delay();


    IIC_SCL_L();
}


/* =========================================================
 * 发送一个Byte
 * ========================================================= */

void IIC_Send_Byte(uint8_t txd)
{
    uint8_t t;


    IIC_SDA_OUT();


    IIC_SCL_L();


    for(t = 0; t < 8; t++)
    {
        if(txd & 0x80)
        {
            IIC_SDA_H();
        }
        else
        {
            IIC_SDA_L();
        }


        txd <<= 1;


        IIC_SCL_H();

        IIC_Delay();


        IIC_SCL_L();

        IIC_Delay();
    }
}


/* =========================================================
 * 读取一个Byte
 * ========================================================= */

uint8_t IIC_Read_Byte(uint8_t ack)
{
    uint8_t i;

    uint8_t receive = 0;


    IIC_SDA_IN();


    for(i = 0; i < 8; i++)
    {
        IIC_SCL_L();


        IIC_Delay();


        IIC_SCL_H();


        receive <<= 1;


        if(IIC_READ_SDA())
        {
            receive++;
        }


        IIC_Delay();
    }


    if(!ack)
    {
        IIC_NAck();
    }
    else
    {
        IIC_Ack();
    }


    return receive;
}
