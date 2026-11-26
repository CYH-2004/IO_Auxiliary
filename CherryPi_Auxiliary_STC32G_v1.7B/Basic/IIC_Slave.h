#ifndef IIC_SLAVE_H
#define IIC_SLAVE_H

#include "SysConfig.h"

// 缓存设定
#define WRITE_BUF_NUM    5
#define WRITE_BUF_ZIZE   64

typedef struct iic_slave_data_info_t
{
    uint16_t reg_addr;
    uint8_t data_count;
    uint8_t buf[WRITE_BUF_ZIZE];
    uint32_t rec_time;
}iic_slave_data_info_t;

extern uint8_t iic_salve_init(uint8_t slave_addr, uint8_t *aux_reg, uint16_t reg_count);
extern uint8_t iic_slave_start(void);
extern uint8_t iic_slave_stop(void);
extern uint8_t get_receive_status(void);
extern void *get_data_info(void);
extern uint8_t confirm_data_process(void);

#endif
