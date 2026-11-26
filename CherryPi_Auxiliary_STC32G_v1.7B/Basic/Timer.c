#include "Timer.h"

uint32_t systick_count;

void n_ms(uint16_t x);
void n_ms_timer_0(void);

void timer4_init(void);
uint32_t get_systick(void);

//延时N-MS子程序********************************************************************
void n_ms(uint16_t x)
{
	uint16_t i;
    for(i=0;i<x;i++)
    {
		n_ms_timer_0();
    }
}

//定时器0的阻塞式1ms延时***********************************************************
void n_ms_timer_0(void)
{  
    TMOD &= 0xF0; // 清除定时器0的控制位  
    TMOD |= 0x01; // 设置定时器0为模式1（16位定时器/计数器模式）  
  
    TL0 = T0MS & 0x00ff;            //reload timer1 low byte
    TH0 = T0MS >> 8;                //reload timer1 high byte

    TR0 = 1; // 启动定时器0  
  
    while(!TF0); // 等待定时器溢出
    
  
    TF0 = 0; // 清除溢出标志  
    TR0 = 0; // 停止定时器0  
}


// 配置使用T4作为systick生成器，时钟源选择系统时钟
// 当前配置为每1ms产生一次中断

void timer4_init(void)
{
    systick_count = 0;
    
    // T4做定时器，1T模式，关闭时钟输出；关闭T3
    T4T3M = 0x20;

    // 设定时钟分频
    TM4PS = 29;
    
    // 定时器预设
    T4L = T4Tick_SET & 0xff;
    T4H = (T4Tick_SET >> 8) & 0xff;
    
    //定时器4开始计数，允许中断
    IE2 |= 0x40;
    T4T3M |= 0x80;
}

//T4中断服务程序
void TM4_Isr() interrupt 20
{
    systick_count++;
}

//获取systick值
//注意数据只有32位，单位为毫秒时49天就会发生溢出
uint32_t get_systick(void) 
{
    return systick_count;
}
