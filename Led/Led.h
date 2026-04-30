//
// Created by Khalaf on 16/04/2026.
//

#ifndef LED_H
#define LED_H

#include "../include/STD_TYPES.h"
#include "../GPIO/GPIO.h"

/* --- Configuration Structure --- */
typedef struct {
    GPIO_Pin_Location_t pin;
} Led_Config_t;

typedef Led_Config_t* Led_Config_Handle_t;

/* --- Function Prototypes --- */
void Led_PrepareConfig(P_void config_out, GPIO_Pin_Location_t led_pin);
void Led_Init(const Led_Config_Handle_t config_h);
void Led_TurnOn(const Led_Config_Handle_t config_h);
void Led_TurnOff(const Led_Config_Handle_t config_h);
void Led_Toggle(const Led_Config_Handle_t config_h);

#endif //LED_H