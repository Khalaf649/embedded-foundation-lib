//
// Created by Khalaf on 16/04/2026.
//

#ifndef BUZZER_H
#define BUZZER_H

#include "../include/STD_TYPES.h"
#include "../GPIO/GPIO.h"

/* --- Configuration Structure --- */
typedef struct {
    GPIO_Pin_Location_t pin;
} Buzzer_Config_t;

typedef Buzzer_Config_t* Buzzer_Config_Handle_t;

/* --- Function Prototypes --- */
void Buzzer_PrepareConfig(P_void config_out, GPIO_Pin_Location_t buzzer_pin);
void Buzzer_Init(const Buzzer_Config_Handle_t config_h);
void Buzzer_TurnOn(const Buzzer_Config_Handle_t config_h);
void Buzzer_TurnOff(const Buzzer_Config_Handle_t config_h);
void Buzzer_Toggle(const Buzzer_Config_Handle_t config_h);

#endif //BUZZER_H