//
// Created by Khalaf on 16/04/2026.
//

// exti.c
// Created by Khalaf on 16/04/2026.

#include "exti.h"
#include "exti_private.h"
#include "../include/BIT_MATH.h"
#include <stddef.h>

// Array to hold user callback functions for each EXTI line
static ExtiCallback ExtiCallbacks[16] = {NULL};

// NVIC IRQ numbers mapping for each EXTI line
static const uint8 ExtiLineNumberNvicMap[16] = {
    6, 7, 8, 9, 10,           // EXTI0 ~ EXTI4
    23, 23, 23, 23, 23,       // EXTI5 ~ EXTI9  → shared EXTI9_5_IRQn
    40, 40, 40, 40, 40, 40    // EXTI10 ~ EXTI15 → shared EXTI15_10_IRQn
};


static void Exti_ClearPending(EXTI_Line_t LineNumber)
{
    SET_BIT(EXTI->PR, LineNumber);
}



void Exti_Init(EXTI_Line_t LineNumber, EXTI_Port_t PortName,
               EXTI_Edge_t EdgeType, ExtiCallback Callback)
{
    if (LineNumber > EXTI_LINE_15) return;

    /* 1. Configure SYSCFG EXTICR register to map GPIO port to this EXTI line
       - Each EXTICR register controls 4 lines (4 bits per line)
       - This tells the EXTI which GPIO port the pin belongs to */
    uint8 sysConfigIndex = LineNumber / 4;
    uint8 fieldIndex     = LineNumber % 4;

    // Clear the 4-bit field first
    WRITE_BIT_FIELD(SYSCFG->EXTICR[sysConfigIndex], 0x0FU, fieldIndex, 4, PortName);

    /* 2. Save the user callback function */
    ExtiCallbacks[LineNumber] = Callback;


    CLR_BIT(EXTI->RTSR, LineNumber);
    CLR_BIT(EXTI->FTSR, LineNumber);

    switch (EdgeType)
    {
        case EXTI_EDGE_RISING:
            SET_BIT(EXTI->RTSR, LineNumber);
            break;

        case EXTI_EDGE_FALLING:
            SET_BIT(EXTI->FTSR, LineNumber);
            break;

        case EXTI_EDGE_BOTH:
            SET_BIT(EXTI->RTSR, LineNumber);
            SET_BIT(EXTI->FTSR, LineNumber);
            break;

        default:
            break;
    }
}

void Exti_Enable(EXTI_Line_t LineNumber)
{
    if (LineNumber > EXTI_LINE_15) return;

    SET_BIT(EXTI->IMR, LineNumber);

    uint8 irqNumber = ExtiLineNumberNvicMap[LineNumber];
    //SET_BIT(NVIC->ISER[irqNumber / 32], (irqNumber % 32));
}


void Exti_Disable(EXTI_Line_t LineNumber)
{
    if (LineNumber > EXTI_LINE_15) return;

    /* Disable interrupt in EXTI_IMR */
    CLR_BIT(EXTI->IMR, LineNumber);

    /* Disable in NVIC */
    uint8 irqNumber = ExtiLineNumberNvicMap[LineNumber];
   // SET_BIT(NVIC->ICER[irqNumber / 32], (irqNumber % 32));
}


void EXTI0_IRQHandler(void)
{
    if (ExtiCallbacks[EXTI_LINE_0]) {
        ExtiCallbacks[EXTI_LINE_0]();
    }
    Exti_ClearPending(EXTI_LINE_0);
}

void EXTI1_IRQHandler(void)
{
    if (ExtiCallbacks[EXTI_LINE_1]) {
        ExtiCallbacks[EXTI_LINE_1]();
    }
    Exti_ClearPending(EXTI_LINE_1);
}

void EXTI2_IRQHandler(void)
{
    if (ExtiCallbacks[EXTI_LINE_2]) {
        ExtiCallbacks[EXTI_LINE_2]();
    }
    Exti_ClearPending(EXTI_LINE_2);
}

void EXTI3_IRQHandler(void)
{
    if (ExtiCallbacks[EXTI_LINE_3]) {
        ExtiCallbacks[EXTI_LINE_3]();
    }
    Exti_ClearPending(EXTI_LINE_3);
}

void EXTI4_IRQHandler(void)
{
    if (ExtiCallbacks[EXTI_LINE_4]) {
        ExtiCallbacks[EXTI_LINE_4]();
    }
    Exti_ClearPending(EXTI_LINE_4);
}

void EXTI9_5_IRQHandler(void)
{
    for (uint8 i = 5; i <= 9; i++)
    {
        if (GET_BIT(EXTI->PR, i))
        {
            if (ExtiCallbacks[i]) {
                ExtiCallbacks[i]();
            }
            Exti_ClearPending(i);
        }
    }
}

void EXTI15_10_IRQHandler(void)
{
    for (uint8 i = 10; i <= 15; i++)
    {
        if (GET_BIT(EXTI->PR, i))
        {
            if (ExtiCallbacks[i]) {
                ExtiCallbacks[i]();
            }
            Exti_ClearPending(i);
        }
    }
}
