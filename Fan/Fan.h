//
// Created by Khalaf on 30/04/2026.
//

#ifndef FAN_H
#define FAN_H
#include "../include/STD_TYPES.h"
#include "../Timer/Timer.h" // Includes your Pwm_Init and CCR_REG macro
#include "../PWM/PWM.h"

/* --- Configuration Structure --- */
typedef struct {
    Tim_Instance_t timer_instance;
    uint8 channel;
} Fan_Config_t;

/* --- Function Prototypes --- */
void Fan_Init(const Fan_Config_t* fan_cfg);
void Fan_SetSpeed(const Fan_Config_t* fan_cfg, uint8 duty_cycle_percent);
#endif //FAN_H
