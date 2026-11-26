#include "IIC_Slave.h"
#include "Uart1.h"
#include "Timer.h"  // 用于记录接收时间戳

bit isda;           //设备地址标志
bit isma_h;         //存储地址高位标志
bit isma_l;         //存储地址低位标志

static uint16_t addr_target;
static uint16_t addr_target_temp;

static uint8_t *storage_target;    // 存储器指针
static uint16_t storage_size;      // 存储器大小

// 缓冲事件记录
volatile uint8_t data_count_temp;
volatile uint8_t buf_count;
volatile uint8_t buf_head_pointer;
volatile uint8_t buf_tail_pointer;
iic_slave_data_info_t write_buf_info[WRITE_BUF_NUM]; 

uint8_t iic_slave_status = 0;   // IIC从机驱动状态

uint8_t iic_salve_init(uint8_t slave_addr, uint8_t *aux_reg, uint16_t reg_count);
uint8_t iic_slave_start(void);
uint8_t iic_slave_stop(void);
uint8_t get_receive_status(void);
void *get_data_info(void);
uint8_t confirm_data_process(void);


// I2C中断服务程序
void I2C_Isr() interrupt 24
{
    if (I2CSLST & STAIF)                         // 处理START事件
    {
        isda = 1;
        isma_h = 1;
        isma_l = 1;
        I2CSLST &= ~0X40;   // 清除标志位
    }
    else if (I2CSLST & RXIF)                    // 处理RECV事件，SLACKO设置为0
    {
        if (isda)                               // 从机地址匹配成功，准备数据接收
        {
            isda = 0;                           // 处理RECV事件（RECV DEVICE ADDR）
        }
        else if (isma_h)                        // 接收目标寄存器高位地址（RECV MEMORY ADDR HIGH）
        {
            isma_h = 0;
            addr_target = (uint16_t)(I2CRXD << 8);
        }
        else if (isma_l)                        // 接收目标寄存器低位地址（RECV MEMORY ADDR LOW）
        {
            isma_l = 0;
            addr_target += I2CRXD;              
            addr_target_temp = addr_target;
            if(addr_target >= storage_size) I2CTXD = 0xFF;    // 访问越界
            else I2CTXD = storage_target[addr_target];        // 预载发送数据
        }
        else                                    // 处理RECV事件（RECV DATA）
        {
            if(addr_target < storage_size)         // 越界检测
            {
                if(buf_count < WRITE_BUF_NUM)  // 缓冲区检测
                {
                    if(data_count_temp < WRITE_BUF_ZIZE)
                    {
                        write_buf_info[buf_tail_pointer].buf[data_count_temp] = I2CRXD;
                        data_count_temp++;
                    }   
                }
            }
        }
        I2CSLST &= ~0X20;   // 清除标志位
    }
    else if (I2CSLST & 0x10)                    // 处理SEND事件
    {
        if (I2CSLST & SLACKI)                   // 主机未确认，放弃发送
        {
            I2CTXD = 0xFF;
        }
        else                                    // 主机已确认，发送数据
        {
            addr_target++;
            if(addr_target >= storage_size) I2CTXD = 0xFF;  // 访问越界
            else
            {
                I2CTXD = storage_target[addr_target];      // 预载发送数据
            } 
        }
        I2CSLST &= ~0x10;   // 清除标志位
    }
    else if ((I2CSLST & 0X08))                  // 处理STOP事件
    {
        
        if(data_count_temp != 0)
        {
            if(buf_count < WRITE_BUF_NUM)
            {
                write_buf_info[buf_tail_pointer].reg_addr = addr_target_temp;
                write_buf_info[buf_tail_pointer].data_count = data_count_temp;
                write_buf_info[buf_tail_pointer].rec_time = get_systick();      // 记录接收时间，如需提高效率可将该行注释
                buf_count++;
                buf_tail_pointer++;
                if(buf_tail_pointer == WRITE_BUF_NUM) buf_tail_pointer = 0;
            }
            data_count_temp = 0;
        }
        I2CSLST &= ~0X08;   // 清除标志位
    }
}

/**
 * @name    iic_salve_init
 * @brief   初始化IIC从机驱动
 * @param   slave_addr：从机地址（7位）
 * @param   reg_data_p：存储区指针
 * @param   reg_num：存储区大小（字节）
 * @return  0：成功  1：存储区指针无效  2：存储区大小无效  3：从机地址无效
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t iic_salve_init(uint8_t slave_addr, uint8_t *aux_reg, uint16_t reg_count)
{
    if(aux_reg == NULL)  return 1;      // 判断存储器指针是否有效
    else storage_target = aux_reg;

    if(reg_count == 0) return 2;        // 存储器大小不能为0
    else storage_size = reg_count;  

    if(slave_addr == 0) return 3;       // 从机地址不能为0

    // 初始化
    data_count_temp = 0;
    buf_head_pointer = 0;
    buf_tail_pointer = 0;
    addr_target = 0;
    buf_count = 0;

    // 选择功能脚前注意提前配置IO模式
    I2C_S1 =1;   //I2C功能脚选择，00:P1.5,P1.4; 01:P2.5,P2.4; 11:P3.2,P3.3
    I2C_S0 =1;

    P3M0 |= 0x0c; P3M1 |= 0x0c;         // 配置P3.2、P3.3为开漏输出模式

    I2CCFG = 0x80;                              //使能I2C从机模式
    I2CSLADR = (slave_addr << 1);               //设置从机设备地址
    I2CSLST = 0x00;
    // I2CSLCR = ESTAI | ERXI | ETXI | ESTOI;      //使能从机模式中断
    
    I2CTXD = storage_target[addr_target];

    iic_slave_status = 1;   // 标记IIC从机已完成初始化

    return 0;
}

/**
 * @name    iic_slave_start
 * @brief   启用IIC从机驱动
 * @param   NULL
 * @return  0：成功  1：IIC从机驱动未初始化  2：未知错误
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t iic_slave_start(void)
{
    if(iic_slave_status == 0) return 1;
    else if(iic_slave_status == 1)
    {
        isda = 1;                                   //用户变量初始化
        isma_h = 1;
        isma_l = 1;
        
        I2CSLCR = ESTAI | ERXI | ETXI | ESTOI;      //使能从机模式中断
        return 0;
    }
    else return 2; 
}

/**
 * @name    iic_slave_stop
 * @brief   停止IIC从机驱动
 * @param   NULL
 * @return  0：成功  1：IIC从机驱动未初始化  2：未知错误
 * @date    日期：2026.11.25
 * @author  作者：HAO
 * @note    注意当前停止驱动前没有判断是否存在未完成的IIC传输
 */
uint8_t iic_slave_stop(void)
{
    if(iic_slave_status == 0) return 1;
    else if(iic_slave_status == 1)
    {
        I2CSLCR = 0x00;      // 关闭从机模式中断
        return 0;
    }
    else return 2; 
}

/**
 * @name    get_receive_status
 * @brief   获取接收数据状态
 * @param   NULL
 * @return  已缓冲的写入请求数量
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t get_receive_status(void)
{
    return buf_count;
}

/**
 * @name    get_data_info
 * @brief   获取接收数据信息
 * @param   NULL
 * @return  void* 请求结构体指针
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
void *get_data_info(void)
{
    if(buf_count == 0) return NULL;
    else return &write_buf_info[buf_head_pointer];
}

/**
 * @name    confirm_data_process
 * @brief   确认数据段处理完成
 * @param   NULL
 * @return  0：成功  1：没有待处理的请求
 * @date    日期：2026.11.19
 * @author  作者：HAO
 * @note    
 */
uint8_t confirm_data_process(void)
{
    if(buf_count)
    {
        buf_count --;
        buf_head_pointer ++;
        if(buf_head_pointer == WRITE_BUF_NUM) buf_head_pointer = 0;
        return 0;
    }
    else
    {
        return 1;
    }
}
