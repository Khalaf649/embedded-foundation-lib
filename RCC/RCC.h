//
// Created by Khalaf on 14/04/2026.
//

#ifndef RCC_H
#define RCC_H
#include "STD_TYPES.h"
/* Bus Indices for the Static Array */
#define RCC_AHB1_BUS       0
#define RCC_AHB2_BUS       1
#define RCC_APB1_BUS       2
#define RCC_APB2_BUS       3

/* Peripheral ID = (Bus_Index * 32) + Bit_Position */
#define RCC_EN_GPIOA      ((RCC_AHB1_BUS * 32) + 0)
#define RCC_EN_GPIOB      ((RCC_AHB1_BUS * 32) + 1)
#define RCC_EN_GPIOC      ((RCC_AHB1_BUS * 32) + 2)
#define RCC_EN_GPIOD      ((RCC_AHB1_BUS * 32) + 3)
#define RCC_EN_GPIOE      ((RCC_AHB1_BUS * 32) + 4)

#define RCC_EN_SYSCFG     ((RCC_APB2_BUS * 32) + 14)

void RCC_EnablePeripheral(uint8 PeripheralId);
void Rcc_Init(void);
#endif //RCC_H
