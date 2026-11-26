#include "Sysconfig.h"

#include "Timer.h"
#include "PWMA.h"
#include "IIC_Slave.h"
#include "Uart1.h"

#include "CherryPi_Aux.h"

sbit    LED_MAIN = P1^5;
// sbit    LCD_BKL = P1^3;
sbit    LED_SIGN = P1^4;

void Delay10ms(void)	//@33.1776MHz
{
	unsigned long edata i;

	_nop_();
	_nop_();
	i = 82942UL;
	while (i) i--;
}

void main(void)
{
    //IO设置，空闲IO均配置为高阻输入
    P0M0 = 0x00; P0M1 = 0xff; 
    P1M0 = 0x38; P1M1 = 0xc7;   // P1.3、P1.4、P1.5配置为推挽输出作LED控制
    P2M0 = 0x00; P2M1 = 0xff; 
    P3M0 = 0x0c; P3M1 = 0xfc;   // P3.0、P3.1配置为准双向口作UART，P3.2、P3.3配置为开漏输出作IIC
    P4M0 = 0x00; P4M1 = 0xff;
    P5M0 = 0x00; P5M1 = 0xff; 
    P6M0 = 0x00; P6M1 = 0xff; 
    P7M0 = 0x00; P7M1 = 0xff;   

    P0IE = 0x00; // 关闭P0口数字输入
    P1IE = 0xff; // 开启P1口数字输入
    P2IE = 0x00; // 关闭P2口数字输入
    P1IE = 0xff; 
    P4IE = 0x00; // 关闭P4口数字输入
    P5IE = 0x00; // 关闭P5口数字输入
    P6IE = 0x00; // 关闭P6口数字输入
    P7IE = 0x00; // 关闭P7口数字输入

    //配置P1.3和P1.5为高速大电流模式
    P1SR &= ~0x28; P1DR &= ~0x28; 

    //配置P3.2和P3.2为高速大电流模式
    // P3SR &= ~0x0c; P3DR &= ~0x0c; 

    WTST = 0;   //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快
    EAXFR = 1;  //扩展寄存器(XFR)访问使能
    CKCON = 0;  //提高访问XRAM速度

    RSTCFG |= 0x10;     //启用P5.4低电平复位

    LED_SIGN = 0;   // 开启工作指示灯
    LED_MAIN = 0;   // 关闭夜灯

    LED_SIGN = 1;

    UART1_config(); // 初始化串口1
    timer4_init();  // 初始化timer4，作systick时钟
    PWMA_Init();    // 初始化PWM

    EA = 1;     //使能总中断

    // 启动IIC_AUX
    cherrypi_aux();
    
    // IIC_AUX异常退出时关闭工作指示灯
    LED_SIGN = 1;   // This line should never be reached.
}



