//
// Created by Khalaf on 16/04/2026.
//

#include "Led.h"
#include <stddef.h> // For NULL

void Led_PrepareConfig(P_void config_out, GPIO_Pin_Location_t buzzer_pin) {
    if (config_out != NULL) {
        Led_Config_Handle_t config = (Led_Config_Handle_t)config_out;
        config->pin = buzzer_pin; // Save the port and pin_number
    }
}

void Led_Init(const Led_Config_Handle_t config_h) {
    if (config_h != NULL) {
        GPIO_PinConfig_t buzzerPinCfg;

        // 1. Prepare the output configuration using your GPIO API
        GPIO_PrepareConfig(&buzzerPinCfg,
                           GPIO_MODE_OUTPUT,
                           GPIO_PULL_NONE,
                           GPIO_SPEED_LOW,
                           GPIO_OTYPE_PP);     // Push-Pull output

        // 2. Pass the address of the pin location and the config struct
        GPIO_InitPin(&(config_h->pin), &buzzerPinCfg);

        // 3. Ensure the buzzer starts turned off
        Led_TurnOff(config_h);
    }
}

void Led_TurnOn(const Led_Config_Handle_t config_h) {
    if (config_h != NULL) {
        // Pass the address of the pin struct to act as the handle
        GPIO_WritePin(&(config_h->pin), GPIO_HIGH);
    }
}

void Led_TurnOff(const Led_Config_Handle_t config_h) {
    if (config_h != NULL) {
        GPIO_WritePin(&(config_h->pin), GPIO_LOW);
    }
}

void Led_Toggle(const Led_Config_Handle_t config_h) {
    if (config_h != NULL) {
        GPIO_TogglePin(&(config_h->pin));
    }
}