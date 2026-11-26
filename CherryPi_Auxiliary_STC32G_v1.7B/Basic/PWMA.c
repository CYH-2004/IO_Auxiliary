#include "PWMA.h"
#include "Timer.h"

#define PWM1_0      0x00	//P:P1.0  N:P1.1
#define PWM1_1      0x01	//P:P2.0  N:P2.1
#define PWM1_2      0x02	//P:P6.0  N:P6.1

#define PWM2_0      0x00	//P:P1.2/P5.4  N:P1.3
#define PWM2_1      0x04	//P:P2.2  N:P2.3
#define PWM2_2      0x08	//P:P6.2  N:P6.3

#define PWM3_0      0x00	//P:P1.4  N:P1.5
#define PWM3_1      0x10	//P:P2.4  N:P2.5
#define PWM3_2      0x20	//P:P6.4  N:P6.5

#define PWM4_0      0x00	//P:P1.6  N:P1.7
#define PWM4_1      0x40	//P:P2.6  N:P2.7
#define PWM4_2      0x80	//P:P6.6  N:P6.7
#define PWM4_3      0xC0	//P:P3.4  N:P3.3

#define ENO1P       0x01
#define ENO1N       0x02
#define ENO2P       0x04
#define ENO2N       0x08
#define ENO3P       0x10
#define ENO3N       0x20
#define ENO4P       0x40
#define ENO4N       0x80

uint16_t PWM1_Duty;
uint16_t PWM2_Duty;
uint16_t PWM3_Duty;
uint16_t PWM4_Duty;

uint8_t PWM1_inc_dir;
uint8_t PWM2_inc_dir;
uint8_t PWM3_inc_dir;
uint8_t PWM4_inc_dir;


void Write_PWMA_ASync(uint8_t addr, uint8_t dat)
{
    while(HSPWMA_ADR & 0x80);
    HSPWMA_DAT = dat;
    HSPWMA_ADR = addr & 0x7F;
}

void PWMA_Init(void)
{
    PWMA_PSCRH = 0x00; //预分频器清0
    PWMA_PSCRL = 0x00;

    PWMA_CCER1 = 0x00; //写 CCMRx 前必须先清零 CCxE 关闭通道
    PWMA_CCER2 = 0x00;
    PWMA_CCMR1 = 0x60; //通道模式配置
    PWMA_CCMR2 = 0x60;
    PWMA_CCMR3 = 0x60;
    PWMA_CCMR4 = 0x60;
    PWMA_CCER1 = 0x55; //配置通道输出使能和极性
    PWMA_CCER2 = 0x55;

    PWMA_CCMR1 |= 0x08; //开启PWMA_CCR1预转载功能(需要CC1E=1才可写)
    PWMA_CCMR2 |= 0x08;
    PWMA_CCMR3 |= 0x08;
    PWMA_CCMR4 |= 0x08;

    PWMA_ARRH = (uint8_t)(PWM_PERIOD >> 8); //设置周期时间
    PWMA_ARRL = (uint8_t)PWM_PERIOD;

    PWMA_ENO = 0x00;
    // PWMA_ENO |= ENO1P; //使能输出
    // PWMA_ENO |= ENO1N; //使能输出
    // PWMA_ENO |= ENO2P; //使能输出
    PWMA_ENO |= ENO2N; //使能输出
    // PWMA_ENO |= ENO3P; //使能输出
    PWMA_ENO |= ENO3N; //使能输出
    // PWMA_ENO |= ENO4P; //使能输出
    // PWMA_ENO |= ENO4N; //使能输出

    PWMA_PS = 0x00;  //高级 PWM 通道输出脚选择位
    PWMA_PS |= PWM2_0; //选择 PWM2_0 通道
    PWMA_PS |= PWM3_0; //选择 PWM3_0 通道

    PWMA_BKR = 0x80;  //使能主输出
    PWMA_CR1 |= 0x81; //使能ARR预装载,开始计时


    //PWM占空比初始化
    PWM1_Duty = 0;
    PWM2_Duty = 0;
    PWM3_Duty = 0;
    PWM4_Duty = 0;
    
    //PWM自动设置方向初始化
    PWM1_inc_dir = 1;
    PWM2_inc_dir = 1;
    PWM3_inc_dir = 1;
    PWM4_inc_dir = 1;
}

void UpdatePwm(void)
{
    PWMA_CCR1H = (uint8_t)(PWM1_Duty >> 8); //设置占空比时间
    PWMA_CCR1L = (uint8_t)(PWM1_Duty);
    PWMA_CCR2H = (uint8_t)(PWM2_Duty >> 8); //设置占空比时间
    PWMA_CCR2L = (uint8_t)(PWM2_Duty);
    PWMA_CCR3H = (uint8_t)(PWM3_Duty >> 8); //设置占空比时间
    PWMA_CCR3L = (uint8_t)(PWM3_Duty);
    PWMA_CCR4H = (uint8_t)(PWM4_Duty >> 8); //设置占空比时间
    PWMA_CCR4L = (uint8_t)(PWM4_Duty);
}

void PWM_auto_set(uint8_t PWM_channel)
{
    switch(PWM_channel)
    {
    case 1:
        //PWM1占空比自动调节
        if(PWM1_inc_dir)
        {
            if(PWM1_Duty < PWM_DUTY_MAX) PWM1_Duty++;
            else PWM1_inc_dir = 0;
        }
        else
        {
            if(PWM1_Duty > 0) PWM1_Duty--;
            else PWM1_inc_dir = 1;
        }
        break;
    case 2:
        //PWM2占空比自动调节
        if(PWM2_inc_dir)
        {
            if(PWM2_Duty < PWM_DUTY_MAX) PWM2_Duty++;
            else PWM2_inc_dir = 0;
        }
        else
        {
            if(PWM2_Duty > 0) PWM2_Duty--;
            else PWM2_inc_dir = 1;
        }
        break;
    case 3:
        //PWM3占空比自动调节
        if(PWM3_inc_dir)
        {
            if(PWM3_Duty < PWM_DUTY_MAX) PWM3_Duty++;
            else PWM3_inc_dir = 0;
        }
        else
        {
            if(PWM3_Duty > 0) PWM3_Duty--;
            else PWM3_inc_dir = 1;
        }
        break;
    case 4:
        //PWM4占空比自动调节
        if(PWM3_inc_dir)
        {
            if(PWM4_Duty < PWM_DUTY_MAX) PWM4_Duty++;
            else PWM4_inc_dir = 0;
        }
        else
        {
            if(PWM4_Duty > 0) PWM4_Duty--;
            else PWM4_inc_dir = 1;
        }
        break;
    }
}

void SetPWM(uint8_t channel, uint16_t duty)
{
    switch(channel)
    {
    case 1:
        PWM1_Duty = duty;
        break;
    case 2:
        PWM2_Duty = duty;
        break;
    case 3:
        PWM3_Duty = duty;
        break;
    case 4:
        PWM4_Duty = duty;
        break;
    }
}