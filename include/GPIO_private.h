//
// Created by Khalaf on 13/04/2026.
//
/* --- GPIO Register Mapping Struct --- */
#include "STD_TYPES.h"
typedef struct {
    volatile uint32 MODER;    /* Mode register */
    volatile uint32 OTYPER;   /* Output type register */
    volatile uint32 OSPEEDR;  /* Output speed register */
    volatile uint32 PUPDR;    /* Pull-up/pull-down register */
    volatile uint32 IDR;      /* Input data register */
    volatile uint32 ODR;      /* Output data register */
    volatile uint32 BSRR;     /* Bit set/reset register */
    volatile uint32 LCKR;     /* Configuration lock register */
    volatile uint32 AFR[2];   /* Alternate function low/high */
} GPIO_RegDef_t;

/* --- GPIO Base Addresses --- */
#define GPIOA_BASE_ADDR    0x40020000U
#define GPIOB_BASE_ADDR    0x40020400U
#define GPIOC_BASE_ADDR    0x40020800U
#define GPIOD_BASE_ADDR    0x40020C00U
#define GPIOE_BASE_ADDR    0x40021000U

#define GPIOA    ((GPIO_RegDef_t *)GPIOA_BASE_ADDR)
#define GPIOB    ((GPIO_RegDef_t *)GPIOB_BASE_ADDR)
#define GPIOC    ((GPIO_RegDef_t *)GPIOC_BASE_ADDR)
#define GPIOD    ((GPIO_RegDef_t *)GPIOD_BASE_ADDR)
#define GPIOE    ((GPIO_RegDef_t *)GPIOE_BASE_ADDR)

static GPIO_RegDef_t* const GPIO_PORT_LOOKUP[] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE
};