//
// Created by Khalaf on 16/04/2026.
//

#ifndef EXTI_PRIVATE_H
#define EXTI_PRIVATE_H
#include "../include/Std_Types.h"

/* EXTI Register Map */
typedef struct {
    volatile uint32 IMR;
    volatile uint32 EMR;
    volatile uint32 RTSR;
    volatile uint32 FTSR;
    volatile uint32 SWIER;
    volatile uint32 PR;
} Exti_RegDef_t;



/* SYSCFG Register Map */
typedef struct {
    volatile uint32 MEMRMP;
    volatile uint32 PMC;
    volatile uint32 EXTICR[4];
    volatile uint32 _reserved[2];
    volatile uint32 CMPCR;
} Syscfg_RegDef_t;

#define EXTI          ((Exti_RegDef_t*)0x40013C00)
#define SYSCFG        ((Syscfg_RegDef_t*)0x40013800)

#endif /* EXTI_PRIVATE_H_ */
