//
// Created by Khalaf on 13/04/2026.
//

#ifndef KEYPAD_H
#define KEYPAD_H
#include "GPIO.h"
#define KEYPAD_ROWS  4
#define KEYPAD_COLS  4

typedef struct {
    GPIO_Pin_Location_t row_pins[KEYPAD_ROWS];
    GPIO_Pin_Location_t col_pins[KEYPAD_COLS];
} Keypad_Config_t;

typedef Keypad_Config_t* Keypad_Config_Handle_t;
typedef uint8_t* Keypad_Data_Handle_t;

void Keypad_Init(const Keypad_Config_Handle_t config_h);
boolean Keypad_GetPressedKey(const Keypad_Config_Handle_t config_h, Keypad_Data_Handle_t key_h);
void Keypad_PrepareConfig(P_void config_out, GPIO_Pin_Location_t rows[KEYPAD_ROWS], GPIO_Pin_Location_t cols[KEYPAD_ROWS]);

#endif //KEYPAD_H
