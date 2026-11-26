#ifndef CHERRI_PI_AUX_H
#define CHERRY_PI_AUX_H

// IIC从机地址
#define IIC_SLAVE_ADDR  0x2A

// 寄存器地址偏移(已暂时删除该功能)
// #define REG_OFFSET      0x0000

// 寄存器容量（字节）
#define REG_COUNT       256

extern void cherrypi_aux(void);

#endif