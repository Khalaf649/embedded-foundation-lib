//
// Created by Khalaf on 29/04/2026.
//


#ifndef NVIC_PRIVATE_H
#define NVIC_PRIVATE_H
#include <Std_Types.h>

/* NVIC Register Map */
typedef struct {
    volatile uint32 ISER[8];
    uint32 _reserved1[24];
    volatile uint32 ICER[8];
} Nvic_RegDef_t;
#define NVIC          ((Nvic_RegDef_t*)0xE000E100)


#endif //NVIC_PRIVATE_H
