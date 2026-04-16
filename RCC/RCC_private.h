//
// Created by Khalaf on 14/04/2026.
//

#ifndef RCC_PRIVATE_H
#define RCC_PRIVATE_H
#include "STD_TYPES.h"
#define RCC_BASE_ADDR       0x40023800UL
typedef struct {
    volatile uint32 CR;              /* 0x00: Clock control register */
    volatile uint32 PLLCFGR;         /* 0x04: PLL configuration register */
    volatile uint32 CFGR;            /* 0x08: Clock configuration register */
    volatile uint32 CIR;             /* 0x0C: Clock interrupt register */
    volatile uint32 AHB1RSTR;        /* 0x10: AHB1 peripheral reset register */
    volatile uint32 AHB2RSTR;        /* 0x14: AHB2 peripheral reset register */
    uint32          Reserved0[2];    /* 0x18 - 0x1C: Reserved gaps */
    volatile uint32 APB1RSTR;        /* 0x20: APB1 peripheral reset register */
    volatile uint32 APB2RSTR;        /* 0x24: APB2 peripheral reset register */
    uint32          Reserved1[2];    /* 0x28 - 0x2C: Reserved gaps */
    volatile uint32 AHB1ENR;         /* 0x30: AHB1 peripheral clock enable register */
    volatile uint32 AHB2ENR;         /* 0x34: AHB2 peripheral clock enable register */
    uint32          Reserved2[2];    /* 0x38 - 0x3C: Reserved gaps */
    volatile uint32 APB1ENR;         /* 0x40: APB1 peripheral clock enable register */
    volatile uint32 APB2ENR;         /* 0x44: APB2 peripheral clock enable register */
    uint32          Reserved3[2];    /* 0x48 - 0x4C: Reserved gaps */
    volatile uint32 AHB1LPENR;       /* 0x50: AHB1 peripheral clock enable in low power mode register */
    volatile uint32 AHB2LPENR;       /* 0x54: AHB2 peripheral clock enable in low power mode register */
    uint32          Reserved4[2];    /* 0x58 - 0x5C: Reserved gaps */
    volatile uint32 APB1LPENR;       /* 0x60: APB1 peripheral clock enable in low power mode register */
    volatile uint32 APB2LPENR;       /* 0x64: APB2 peripheral clock enable in low power mode register */
    uint32          Reserved5[2];    /* 0x68 - 0x6C: Reserved gaps */
    volatile uint32 BDCR;            /* 0x70: Backup domain control register */
    volatile uint32 CSR;             /* 0x74: Clock control & status register */
    uint32          Reserved6[2];    /* 0x78 - 0x7C: Reserved gaps */
    volatile uint32 SSCGR;           /* 0x80: Spread spectrum clock generation register */
    volatile uint32 PLLI2SCFGR;      /* 0x84: PLLI2S configuration register */
    uint32          Reserved7;       /* 0x88: Reserved gap */
    volatile uint32 DCKCFGR;         /* 0x8C: Dedicated clocks configuration register */
} RCC_RegDef_t;

#define RCC    ((RCC_RegDef_t *)RCC_BASE_ADDR)


#endif //RCC_PRIVATE_H
