//
// Created by Khalaf on 13/04/2026.
//
#include "SevenSeg.h"
#include "SevenSeg_private.h"
#include <stddef.h>

void SevenSeg_WriteDigit(const SevenSeg_Config_Handle_t config_h, uint8 digit) {
    if (config_h == NULL || digit > 9) return;

    /* 1. Get the base pattern */
    uint8 pattern = SevenSeg_Patterns[digit];

    /* 2. If it's Common Anode, flip all bits */
    if (config_h->type == SEVENSEG_COMMON_ANODE) {
        pattern = ~pattern;
    }

    /* 3. Send to GPIO */
    for (uint8 i = 0; i < SEVENSEG_PIN_COUNT; i++) {
        GPIO_PinState_t state = (pattern & (1 << i)) ? GPIO_HIGH : GPIO_LOW;
        GPIO_WritePin(&(config_h->segments[i]), state);
    }
}

void SevenSeg_PrepareConfig(P_void config_out, GPIO_Pin_Location_t segments[SEVENSEG_PIN_COUNT], SevenSeg_Type_t type) {
    if (config_out == NULL) return;

    /* Cast P_void back to our private struct type */
    SevenSeg_Config_t* config = (SevenSeg_Config_t*)config_out;

    config->type = type;
    for (uint8 i = 0; i < SEVENSEG_PIN_COUNT; i++) {
        config->segments[i] = segments[i];
    }
}
void SevenSeg_Init(const SevenSeg_Config_Handle_t config_h) {
    if (config_h == NULL) return;


    SevenSeg_Config_t* config = (SevenSeg_Config_t*)config_h;
    GPIO_PinConfig_t seg_cfg;

    /* Configure segments as Output, Push-Pull, No Pulls */
    GPIO_PrepareConfig(&seg_cfg, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW, GPIO_OTYPE_PP);

    for (uint8 i = 0; i < SEVENSEG_PIN_COUNT; i++) {
        GPIO_InitPin(&(config->segments[i]), &seg_cfg);
    }

    /* Ensure display starts empty */
    SevenSeg_Clear(config_h);
}
void SevenSeg_Clear(const SevenSeg_Config_Handle_t config_h) {
    if (config_h == NULL) return;

    SevenSeg_Config_t* config = (SevenSeg_Config_t*)config_h;

    /* For Common Cathode, LOW is OFF. For Common Anode, HIGH is OFF. */
    GPIO_PinState_t off_state = (config->type == SEVENSEG_COMMON_CATHODE) ? GPIO_LOW : GPIO_HIGH;

    for (uint8 i = 0; i < SEVENSEG_PIN_COUNT; i++) {
        GPIO_WritePin(&(config->segments[i]), off_state);
    }
}
