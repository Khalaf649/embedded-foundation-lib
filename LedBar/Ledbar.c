//
// Created by Khalaf on 14/04/2026.
//
#include "LedBar.h"
#include <stddef.h>
#include "../GPIO/GPIO.h"

void LedBar_PrepareConfig(P_void config_out, GPIO_Pin_Location_t progress[MAX_PROGRESS_LEDS], GPIO_Pin_Location_t success, GPIO_Pin_Location_t alarm) {
    if (config_out == NULL) {
        return;
    }

    /* Cast the opaque P_void back to our known struct type */
    LedBar_Config_t* config = (LedBar_Config_t*)config_out;

    /* Build the configuration in memory */
    for (uint8 i = 0; i < MAX_PROGRESS_LEDS; i++) {
        config->progress_pins[i] = progress[i];
    }
    config->success_pin = success;
    config->alarm_pin   = alarm;
}

void LedBar_Init(const LedBar_Config_Handle_t config_h) {
    if (config_h == NULL) {
        return;
    }

    GPIO_PinConfig_t led_cfg;

    /* Prepare standard output configuration for all LEDs/Buzzer */
    GPIO_PrepareConfig(&led_cfg, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW, GPIO_OTYPE_PP);

    /* Initialize Progress LEDs and ensure they start OFF */
    for (uint8 i = 0; i < MAX_PROGRESS_LEDS; i++) {
        GPIO_InitPin(&(config_h->progress_pins[i]), &led_cfg);
        GPIO_WritePin(&(config_h->progress_pins[i]), GPIO_LOW);
    }

    /* Initialize Status LEDs/Buzzer and ensure they start OFF */
    GPIO_InitPin(&(config_h->success_pin), &led_cfg);
    GPIO_WritePin(&(config_h->success_pin), GPIO_LOW);

    GPIO_InitPin(&(config_h->alarm_pin), &led_cfg);
    GPIO_WritePin(&(config_h->alarm_pin), GPIO_LOW);
}

void LedBar_SetProgress(const LedBar_Config_Handle_t config_h, uint8 count) {
    if (config_h == NULL) {
        return;
    }

    /* Light up LEDs up to 'count', turn off the rest */
    for (uint8 i = 0; i < MAX_PROGRESS_LEDS; i++) {
        if (i < count) {
            GPIO_WritePin(&(config_h->progress_pins[i]), GPIO_HIGH);
        } else {
            GPIO_WritePin(&(config_h->progress_pins[i]), GPIO_LOW);
        }
    }
}

void LedBar_SetStatus(const LedBar_Config_Handle_t config_h, LedBar_Status_t status) {
    if (config_h == NULL) {
        return;
    }

    /* Enforce strict states to prevent hardware logic conflicts */
    switch (status) {
        case LEDBAR_STATUS_SUCCESS:
            GPIO_WritePin(&(config_h->success_pin), GPIO_HIGH);
            GPIO_WritePin(&(config_h->alarm_pin), GPIO_LOW);
            LedBar_SetProgress(config_h, 0);
            break;

        case LEDBAR_STATUS_ALARM:
            GPIO_WritePin(&(config_h->success_pin), GPIO_LOW);
            GPIO_WritePin(&(config_h->alarm_pin), GPIO_HIGH);
            LedBar_SetProgress(config_h, 0);
            break;

        case LEDBAR_STATUS_IDLE:
        default:
            /* Safe state: Turn everything off */
            GPIO_WritePin(&(config_h->success_pin), GPIO_LOW);
            GPIO_WritePin(&(config_h->alarm_pin), GPIO_LOW);
            LedBar_SetProgress(config_h, 0);
            break;
    }
}