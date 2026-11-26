#ifndef PWMA_H
#define PWMA_H

#include "SysConfig.h"

#define PWM_PERIOD      255    //设置周期值
#define PWM_DUTY_MAX    400    //设置占空比最大值

extern void PWMA_Init(void);
extern void UpdatePwm(void);

extern void PWM_auto_set(uint8_t PWM_channel);

extern void SetPWM(uint8_t channel, uint16_t duty);

#endif