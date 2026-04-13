//
// Created by Khalaf on 13/04/2026.
//

#ifndef KEYPAD_PRIVATE_H
#define KEYPAD_PRIVATE_H
# include "keypad.h"
#include "STD_TYPES.h"

static const uint8 Keypad_Map[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

#endif //KEYPAD_PRIVATE_H
