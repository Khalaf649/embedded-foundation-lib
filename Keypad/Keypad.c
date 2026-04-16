//
// Created by Khalaf on 13/04/2026.
//
#include "Keypad.h"
#include "Keypad_private.h"
#include <stddef.h>
#include "../GPIO/GPIO.h"
void Keypad_Init(const Keypad_Config_Handle_t config_h) {
    if (config_h == NULL) return;

    GPIO_PinConfig_t row_cfg, col_cfg;

    /* Rows: Output, Push-Pull */
    GPIO_PrepareConfig(&row_cfg, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW, GPIO_OTYPE_PP);
    /* Cols: Input, Pull-Up */
    GPIO_PrepareConfig(&col_cfg, GPIO_MODE_INPUT, GPIO_PULL_UP, GPIO_SPEED_LOW, GPIO_OTYPE_PP);

    for (uint8 i = 0; i < KEYPAD_ROWS; i++) {
        GPIO_InitPin(&(config_h->row_pins[i]), &row_cfg);
        GPIO_WritePin(&(config_h->row_pins[i]), GPIO_HIGH);
    }

    for (uint8 i = 0; i < KEYPAD_COLS; i++) {
        GPIO_InitPin(&(config_h->col_pins[i]), &col_cfg);
    }
}

boolean Keypad_GetPressedKey(const Keypad_Config_Handle_t config_h, Keypad_Data_Handle_t key_h) {
    boolean isPressed = FALSE;

    if (config_h == NULL || key_h == NULL) return FALSE;

    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        /* Ground row r */
        GPIO_WritePin(&(config_h->row_pins[r]), GPIO_LOW);

        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if (GPIO_ReadPin(&(config_h->col_pins[c])) == GPIO_LOW) {

                /* DELAY FOR 30MS */


                if (GPIO_ReadPin(&(config_h->col_pins[c]))== GPIO_LOW) {
                    *key_h = Keypad_Map[r][c];
                    isPressed = TRUE;

                    /* Wait for release */
                    while (GPIO_ReadPin((GPIO_Pin_Handle_t)&(config_h->col_pins[c])) == GPIO_LOW);

                }



            }
        }

        /* Set row r back to HIGH */
        GPIO_WritePin(&(config_h->row_pins[r]), GPIO_HIGH);

        if (isPressed) break;
    }

    return isPressed;
}

void Keypad_PrepareConfig(P_void config_out, GPIO_Pin_Location_t rows[KEYPAD_ROWS], GPIO_Pin_Location_t cols[KEYPAD_ROWS]) {

    if (config_out == NULL) return;

    Keypad_Config_t* config = (Keypad_Config_t*)config_out;

    /* Copy row configurations */
    for (uint8 i = 0; i < KEYPAD_ROWS; i++) {
        config->row_pins[i]=rows[i];
    }

    /* Copy column configurations */
    for (uint8 i = 0; i < KEYPAD_COLS; i++) {
        config->col_pins[i]=cols[i];

    }
}