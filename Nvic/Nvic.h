//
// Created by Khalaf on 29/04/2026.
//

#ifndef NVIC_H
#define NVIC_H
#include "STD_TYPES.h"
#define ENTER_CRITICAL()  __asm volatile ("cpsid i" : : : "memory")
#define EXIT_CRITICAL()   __asm volatile ("cpsie i" : : : "memory")
void Nvic_EnableInterrupt(uint8 IRQn);
void Nvic_DisableInterrupt(uint8 IRQn);
#endif //NVIC_H
