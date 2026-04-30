//
// Created by Khalaf on 30/04/2026.
//

#ifndef TIMER_PRIVATE_H
#define TIMER_PRIVATE_H
#include <STD_TYPES.h>

typedef struct {
    volatile uint32 CR[2];       /* 0x00 - 0x04: Control registers 1 & 2         */
    volatile uint32 SMCR;        /* 0x08 - Slave mode control register           */
    volatile uint32 DIER;        /* 0x0C - DMA/Interrupt enable register         */
    volatile uint32 SR;          /* 0x10 - Status register                       */
    volatile uint32 EGR;         /* 0x14 - Event generation register             */
    volatile uint32 CCMR[2];     /* 0x18 - 0x1C: Capture/compare mode 1 & 2      */
    volatile uint32 CCER;        /* 0x20 - Capture/compare enable register       */
    volatile uint32 CNT;         /* 0x24 - Counter register                      */
    volatile uint32 PSC;         /* 0x28 - Prescaler                             */
    volatile uint32 ARR;         /* 0x2C - Auto-reload register                  */
    volatile uint32 _reserved1;  /* 0x30 - (Used as RCR in Advanced Timers)      */
    volatile uint32 CCR[4];      /* 0x34 - 0x40: Capture/compare registers 1-4   */
    volatile uint32 _reserved2;  /* 0x44 - Padding                               */
    volatile uint32 DCR;         /* 0x48 - DMA control register                  */
    volatile uint32 DMAR;        /* 0x4C - DMA address for full transfer         */
    volatile uint32 OR;          /* 0x50 - Option register (TIM2/TIM5/TIM11)     */
} TimerType;
/* Timer Peripheral Base Addresses */
#define TIM2_BASE_ADDR   0x40000000UL
#define TIM3_BASE_ADDR   0x40000400UL
#define TIM4_BASE_ADDR   0x40000800UL
#define TIM5_BASE_ADDR   0x40000C00UL

/* Timer Instance Pointers */
#define TIM2             ((TimerType*)TIM2_BASE_ADDR)
#define TIM3             ((TimerType*)TIM3_BASE_ADDR)
#define TIM4             ((TimerType*)TIM4_BASE_ADDR)
#define TIM5             ((TimerType*)TIM5_BASE_ADDR)


/* CR1 bit positions */
#define TIMER_CR1_CEN      0U    /* Counter enable              */
#define TIMER_CR1_URS      2U    /* Update request source       */
#define TIMER_CR1_OPM      3U    /* One-pulse mode              */
#define TIMER_CR1_ARPE     7U    /* Auto-reload preload enable  */

/* DIER bit positions */
#define TIMER_DIER_UIE     0U    /* Update interrupt enable     */

/* SR bit positions */
#define TIMER_SR_UIF       0U    /* Update interrupt flag       */

/* EGR bit positions */
#define TIMER_EGR_UG       0U    /* Update generation           */

#endif /* TIMER_PRIVATE_H */
