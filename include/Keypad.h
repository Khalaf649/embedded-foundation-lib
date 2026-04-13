//
// Created by Khalaf on 13/04/2026.
//

#ifndef KEYPAD_H
#define KEYPAD_H
#include "GPIO.h"
#define KEYPAD_ROWS    4
#define KEYPAD_COLS    4

typedef struct {
    GPIO_Pin_Location_t row_pins[KEYPAD_ROWS];
    GPIO_Pin_Location_t col_pins[KEYPAD_COLS];
} Keypad_Config_t;

void Keypad_Init(const Keypad_Config_t* config);
uint8_t Keypad_GetPressedKey(void);



#endif //KEYPAD_H
