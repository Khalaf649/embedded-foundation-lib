//
// Created by Khalaf on 14/04/2026.
//

#ifndef LEDBAR_H
#define LEDBAR_H
#include "STD_TYPES.h"
#include "../GPIO/GPIO.h"
#define MAX_PROGRESS_LEDS  4
typedef enum {
    LEDBAR_STATUS_IDLE = 0,  /* Both LEDs OFF (Locked, normal state) */
    LEDBAR_STATUS_SUCCESS,   /* Success LED ON, Alarm OFF */
    LEDBAR_STATUS_ALARM      /* Success LED OFF, Alarm ON/Buzzer ON */
} LedBar_Status_t;
typedef struct {
    GPIO_Pin_Location_t progress_pins[MAX_PROGRESS_LEDS];
    GPIO_Pin_Location_t success_pin;
    GPIO_Pin_Location_t alarm_pin;
} LedBar_Config_t;
typedef LedBar_Config_t* LedBar_Config_Handle_t;
void LedBar_PrepareConfig(P_void config_out, GPIO_Pin_Location_t progress[MAX_PROGRESS_LEDS], GPIO_Pin_Location_t success, GPIO_Pin_Location_t alarm);
void LedBar_Init(const LedBar_Config_Handle_t config_h);
void LedBar_SetProgress(const LedBar_Config_Handle_t config_h, uint8 count);
void LedBar_SetStatus(const LedBar_Config_Handle_t config_h, LedBar_Status_t status);

#endif //LEDBAR_H
