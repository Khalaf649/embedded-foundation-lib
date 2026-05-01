//
// Created by Khalaf on 16/04/2026.
//

#ifndef UTILS_H
#define UTILS_H
#include "Std_Types.h"

// Initializes the SysTick timer
void SysTick_Init(void);

// Hardware-accurate millisecond delay
void floatToStr(float val, char* data);
void IntToString(uint32 num, char* str);
#endif //UTILS_H
