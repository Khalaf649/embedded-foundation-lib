//
// Created by Khalaf on 16/04/2026.
//

#ifndef UTILS_H
#define UTILS_H
#include "Std_Types.h"

// Initializes the SysTick timer
void SysTick_Init(void);

// Hardware-accurate millisecond delay
void delay_ms(uint32 ms);
void floatToStr(float val, char* data);
#endif //UTILS_H
