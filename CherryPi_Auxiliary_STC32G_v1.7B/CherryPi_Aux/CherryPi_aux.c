#include "CherryPi_aux.h"
#include "SysConfig.h"

#include "PWMA.h"
#include "Uart1.h"
#include "IIC_Slave.h"

#include "Timer.h"

#include <stdio.h>

/*
 *  [固件ID寄存器]
*/
#define ID_REG_ADDR         0x0003
#define ID_REG_SIZE         0x0004
#define ID_REG_VALUE_H      0xBD00  // B:B版本、D:适用于CheeryPi-TFT的定制版
#define ID_REG_VALUE_L      0x0170

/*
 *  [PWM控制寄存器]
*/

// 灯丝 PWM控制寄存器
#define LED_PWM_REG_ADDR    0x0010
#define LED_PWM_REG_SIZE    0x0001  //1字节

// 背光 PWM控制寄存器
#define BLK_PWM_REG_ADDR    0x0012
#define BLK_PWM_REG_SIZE    0x0001  //1字节

/*
 *  [端口数据及控制寄存器]
 *  采用16位寄存器框架，以便于移植至STM32等其他使用16位端口数据寄存器的MCU
*/

// P0、P1 端口数据寄存器
#define IO_REG_0_ADDR         0x0020
#define IO_REG_0_SIZE         0x0002

// P0、P1 端口模式配置寄存器
#define IO_MODE_REG_0_ADDR    0x0022
#define IO_MODE_REG_0_SIZE    0x0008

// P3、P2 端口数据寄存器
#define IO_REG_1_ADDR         0x0030
#define IO_REG_1_SIZE         0x0002

// P3、P2 端口模式配置寄存器
#define IO_MODE_REG_1_ADDR    0x0032
#define IO_MODE_REG_1_SIZE    0x0008

// 休眠控制寄存器
#define SLEEP_REG_ADDR        0x00F0
#define SLEEP_REG_SIZE        0x0001

/*
 *  [手动IO控制]
*/

sbit    LED_main = P1^5;    // 陶瓷灯丝
sbit    LED_sign = P1^4;    // 3.3V指示灯
sbit    LCD_BKL = P1^3;     // LCD背光
sbit    PIN_INT = P3^6;     // 中断唤醒

uint8_t aux_reg[REG_COUNT]; // 虚拟寄存器

char str_buf[256];          // 临时字符串缓冲区

void cherrypi_aux(void);
uint8_t get_reg_data_16(uint16_t reg_addr, uint8_t *reg_data_p, uint8_t reg_num);
uint8_t set_reg_data_16(uint16_t reg_addr, uint8_t *reg_data_p, uint8_t reg_num);

uint8_t set_P1_mode(uint8_t io_mode, uint8_t io_index);
uint8_t set_P3_mode(uint8_t io_mode, uint8_t io_index);

void start_sleep_mode(void);

// 辅助控制器主程序
void cherrypi_aux(void)
{
    uint8_t data_temp8;
    uint16_t data_temp16;
    uint16_t i;

    // uint8_t *reg_buf;
    iic_slave_data_info_t *data_info;

    // 配置INT引脚模式
    P3M0 |= 0x40; P3M1 |= 0x40;     // 配置P3.6为开漏输出模式
    P3PU |= 0x40;                   // 启用P3.6内部上拉电阻

    // 初始化iic从机驱动
    iic_salve_init(IIC_SLAVE_ADDR, aux_reg, REG_COUNT);

    // 初始化寄存器组
    for(i = 0; i < REG_COUNT; i++)
    {
        aux_reg[i] = 0x00;
    }
    aux_reg[IO_REG_0_ADDR] = 0xFF;
    aux_reg[IO_REG_0_ADDR + 1] = 0xFF;
    aux_reg[IO_REG_1_ADDR] = 0xFF;
    aux_reg[IO_REG_1_ADDR + 1] = 0xFF;

    // 载入固件ID
    aux_reg[ID_REG_ADDR] =(ID_REG_VALUE_H >> 8) & 0x00FF;
    aux_reg[ID_REG_ADDR + 1] = ID_REG_VALUE_H & 0x00FF;
    aux_reg[ID_REG_ADDR + 2] = (ID_REG_VALUE_L >> 8) & 0x00FF;
    aux_reg[ID_REG_ADDR + 3] = ID_REG_VALUE_L & 0x00FF;
    
    // 开启驱动（开IIC中断）
    if(iic_slave_start() == 0)
    {
        sprintf(str_buf, "[STC_AUX]CherryPi_Aux Start(%04x%04x)", ID_REG_VALUE_H, ID_REG_VALUE_L);
        PrintString1(str_buf);
    } 
    else PrintString1("[STC_AUX]CherryPi_Aux ERROR.\n");

    // 主任务处理
    while(1)
    {
        // 获取待处理数据信息
        data_info = get_data_info();
        if(data_info != NULL)
        {
            // PrintString1("Received write request.\n");
            if(data_info->reg_addr == LED_PWM_REG_ADDR)         // 灯丝PWM控制寄存器
            {   
                data_temp8 = data_info->buf[0];
                data_temp16 = ((uint16_t)data_temp8 * PWM_PERIOD) / 255 ;
                SetPWM(3, data_temp16);
                UpdatePwm();
                set_reg_data_16(LED_PWM_REG_ADDR, &data_temp8, 1);  // 将缓冲区数据同步至寄存器组

                // PrintString1("Updated LED.\n");
            }
            else if(data_info->reg_addr == BLK_PWM_REG_ADDR)    // 背光PWM控制寄存器
            {
                data_temp8 = data_info->buf[0];
                data_temp16 = ((uint16_t)data_temp8 * PWM_PERIOD) / 255 ;
                SetPWM(2, data_temp16);
                UpdatePwm();
                set_reg_data_16(BLK_PWM_REG_ADDR, &data_temp8, 1);  // 将缓冲区数据同步至寄存器组

                // PrintString1("Updated BLK.\n");
            }
            else if(data_info->reg_addr == IO_REG_0_ADDR)         // 端口数据寄存器0
            {
                // 此处为临时设计（当前版本硬件设计上仅有P10、P11两个自由IO），等待后续重构
                data_temp8 = data_info->buf[0];
                P10 = data_temp8 & 0x01;
                P11 = data_temp8 & 0x02;

                data_info->buf[0] = P1;
                data_info->buf[0] |= 0xFC;
                data_info->buf[1] = 0xFF;       // P0 端口未启用
                
                // 更新寄存器组
                set_reg_data_16(IO_REG_0_ADDR, data_info->buf, 2);

                // PrintString1("Received P1 data write request.");
            }
            else if(data_info->reg_addr == IO_MODE_REG_0_ADDR)    // 端口模式配置寄存器0
            {
                // 此处为临时设计（当前版本硬件设计上仅有P10、P11两个自由IO），等待后续重构
                if(data_info->data_count >= 4)
                {
                    data_temp8 = data_info->buf[3];
                    set_P1_mode((data_temp8 & 0x0F), 0);
                    set_P1_mode((data_temp8 & 0xF0) >> 4, 1);
                    
                    data_info->buf[0] = 0x00;
                    data_info->buf[1] = 0x00;
                    data_info->buf[2] = 0x00;

                    // 更新寄存器组
                    set_reg_data_16(IO_MODE_REG_0_ADDR, data_info->buf, 4);

                    // PrintString1("Received P1 config write request.");
                }
            }
            else if(data_info->reg_addr == IO_REG_1_ADDR)         // 端口数据寄存器1
            {
                // 暂时仅P3.4、P3.5可用
                data_temp8 = data_info->buf[0];
                P34 = data_temp8 & 0x10;
                P35 = data_temp8 & 0x20;

                data_info->buf[0] = P3;
                data_info->buf[0] |= 0xCF;
                data_info->buf[1] = 0xFF;       // P2 端口未启用
                
                // 更新寄存器组
                set_reg_data_16(IO_REG_1_ADDR, data_info->buf, 2);

                // PrintString1("Received P3 data write request.");
            }
            else if(data_info->reg_addr == IO_MODE_REG_1_ADDR)    // 端口模式配置寄存器1
            {
                // 暂时仅P3.4、P3.5可用
                if(data_info->data_count >= 4)
                {
                    data_temp8 = data_info->buf[1];
                    set_P3_mode((data_temp8 & 0x0F), 4);
                    set_P3_mode((data_temp8 & 0xF0) >> 4, 5);
                    
                    data_info->buf[0] = 0x00;
                    data_info->buf[2] = 0x00;
                    data_info->buf[3] = 0x00;

                    // 更新寄存器组
                    set_reg_data_16(IO_MODE_REG_1_ADDR, data_info->buf, 4);

                    // PrintString1("Received P3 config write request.");
                }
            }
            else if(data_info->reg_addr == SLEEP_REG_ADDR)  // 休眠控制寄存器
            {
                if(data_info->buf[0] == 0x01)
                {
                    // 停止IIC从机
                    iic_slave_stop();

                    LED_sign = 1;   // 关闭运行指示灯

                    // 等待INT引脚恢复高电平
                    while(PIN_INT == 0)
                    {
                        PIN_INT = 1;
                        n_ms(5);
                    }

                    // 进入睡眠模式
                    // PrintString1("Start sleep.\n");
                    start_sleep_mode();

                    LED_sign = 0;   // 睡眠已唤醒，开启运行指示灯

                    // 恢复IIC从机
                    // PrintString1("Exit sleep.\n");
                    iic_slave_start();
                }
            }
            else
            {
                PrintString1("[STC_AUX]Invalid register address.\n");
            }

            // 确认数据处理完成
            confirm_data_process();
        }

        // 更新IO端口数据寄存器
        data_temp8 = P1;
        data_temp8 &= 0x03;
        data_temp8 |= 0xFC;
        set_reg_data_16(IO_REG_0_ADDR, &data_temp8, 1);

        data_temp8 = P3;
        data_temp8 &= 0x30;
        data_temp8 |= 0xCF;
        set_reg_data_16(IO_REG_1_ADDR, &data_temp8, 1);
    }
}

/**
 * @name    get_reg_data_16
 * @brief   获取指定寄存器数据
 * @param   reg_addr：寄存器地址（16位寻址）
 * @param   reg_data_p：读取缓冲区指针
 * @param   reg_num：读取寄存器数量（字节）
 * @return  0：成功     1：地址越界
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t get_reg_data_16(uint16_t reg_addr, uint8_t *reg_data_p, uint8_t reg_num)
{
    uint8_t i;
    
    if(reg_addr < REG_COUNT)
    {
        if((reg_addr + reg_num) < REG_COUNT)
        {
            for(i = 0; i < reg_num; i++)
            {
                reg_data_p[i] = aux_reg[reg_addr + i];
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

/**
 * @name    set_reg_data_16
 * @brief   设置指定寄存器数据
 * @param   reg_addr：寄存器地址（16位寻址）
 * @param   reg_data_p：写入缓冲区指针
 * @param   reg_num：写入寄存器数量（字节）
 * @return  0：成功     1：地址越界
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t set_reg_data_16(uint16_t reg_addr, uint8_t *reg_data_p, uint8_t reg_num)
{
    uint8_t i;
    
    if(reg_addr < REG_COUNT)    // 越界检测
    {
        if((reg_addr + reg_num) < REG_COUNT)    // 越界检测
        {
            for(i = 0; i < reg_num; i++)
            {
                aux_reg[reg_addr + i] = reg_data_p[i];
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

/**
 * @name    set_P1_mode
 * @brief   设置P1端口模式
 * @param   io_mode：IO模式    0-高阻输入    1-推挽输出    2-开漏输出    3-准双向     4-ADC
 * @param   io_index：IO索引
 * @return  0：成功     1：无效参数
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t set_P1_mode(uint8_t io_mode, uint8_t io_index)
{
    uint8_t io_value;

    if(io_index > 7) return 1;
    else io_value = 0x01 << io_index;

    if(io_mode == 4)    // P0.0和P0.1可配置使用片内ADC
    {
        // ADC功能后续更新
        return 1;
    }
    else    // 普通IO模式
    {
        switch(io_mode)
        {
            case 0:     // 高阻输入
                P1M0 &= ~io_value;  P1M1 |= io_value; 
                break;
            case 1:     // 推挽输出
                P1M0 |= io_value;   P1M1 &= ~io_value; 
                break;
            case 2:     // 开漏输出
                P1M0 |= io_value;   P1M1 |= io_value;
                break;
            case 3:     // 准双向
                P1M0 &= ~io_value;  P1M1 &= ~io_value; 
                break;
            default:
                return 1;
        }
    }
    return 0;
}

/**
 * @name    set_P3_mode
 * @brief   设置P3端口模式
 * @param   io_mode：IO模式    0-高阻输入    1-推挽输出    2-开漏输出    3-准双向
 * @param   io_index：IO索引
 * @return  0：成功     1：无效参数
 * @date    日期：2026.11.26
 * @author  作者：HAO
 * @note    
 */
uint8_t set_P3_mode(uint8_t io_mode, uint8_t io_index)
{
    uint8_t io_value;

    if(io_index > 7) return 1;
    else io_value = 0x01 << io_index;

    switch(io_mode)
    {
        case 0:     // 高阻输入
            P3M0 &= ~io_value;  P3M1 |= io_value; 
            break;
        case 1:     // 推挽输出
            P3M0 |= io_value;   P3M1 &= ~io_value; 
            break;
        case 2:     // 开漏输出
            P3M0 |= io_value;   P3M1 |= io_value;
            break;
        case 3:     // 准双向
            P3M0 &= ~io_value;  P3M1 &= ~io_value; 
            break;
        default:
            return 1;
    }
    return 0;
}

/**
 * @name    start_sleep_mode
 * @brief   进入睡眠模式
 * @param   NULL
 * @return  NULL
 * @date    日期：2026.11.26
 * @author  作者：HAO
 * @note    
 */
void start_sleep_mode(void)
{
    EX2 = 1;    // 使能 INT2 下降沿中断
    PD = 1;     // Sleep
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();

}

/********************* INT2中断函数 *************************/
void INT2_int (void) interrupt 10     //进中断时已经清除标志
{
    EX2 = 0;    //INT2 Disable
}
