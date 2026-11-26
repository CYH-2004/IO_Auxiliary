#ifndef __SysConfig_H
#define __SysConfig_H

// ----------------------------公共头文件引用-----------------------------
#include "STC32G.h"
#include <stdio.h>
// #include <string.h>
// ------------------------------*********-------------------------------

#define     MAIN_Fosc   33177600UL

// ------------------------------*********-------------------------------

#define TRUE    1
#define FALSE   0

// ----------------------------数据类型宏定义-----------------------------
#define     uint8_t     unsigned char
#define     uint16_t    unsigned int
#define     uint32_t    unsigned long

#define     int8_t      char
#define     int16_t     int
#define     int32_t     long

#define     u8     unsigned char
#define     u16    unsigned int
#define     u32    unsigned long

//适配旧程序宏，建议使用上面的新宏定义
#define     uint        unsigned short
#define     uchar       unsigned char


typedef struct area_type
{
    uint16_t x_start;
    uint16_t y_start;
    uint16_t x_end;
    uint16_t y_end;
}area_t;

// ------------------------------*********-------------------------------

// ----------------------------User Define ------------------------------

#define     LED_BREATHING       0
#define     LED_BREATHING_TIME  500

// ----------------------------Test Define ------------------------------

#endif
