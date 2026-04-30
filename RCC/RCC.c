//
// Created by Khalaf on 14/04/2026.
//
#include "RCC.h"

#include "BIT_MATH.h"
#include "RCC_private.h"

void RCC_EnablePeripheral(uint8 PeripheralId) {
    uint8 BusId  = PeripheralId / 32;
    uint8 BitPos = PeripheralId % 32;

    switch(BusId) {
        case 0: SET_BIT(RCC->AHB1ENR, BitPos); break;
        case 1: SET_BIT(RCC->AHB2ENR, BitPos); break;
        case 2: SET_BIT(RCC->APB1ENR, BitPos); break;
        case 3: SET_BIT(RCC->APB2ENR, BitPos); break;
        default: break;
    }
}
void Rcc_Init(void) {
    SET_BIT(RCC->CR, 0);
    SET_BIT(RCC->APB2ENR, 2);   // Enable GPIOA clock
    SET_BIT(RCC->APB2ENR, 4);
    SET_BIT(RCC->APB2ENR, 8);   // Enable ADC1 clock
    SET_BIT(RCC->APB1ENR, 0);   // Enable TIM2 clock
    SET_BIT(RCC->APB1ENR, 1);   // Enable TIM3 clock
    SET_BIT(RCC->APB1ENR, 2);   // Enable TIM4 clock
    SET_BIT(RCC->APB1ENR, 3);   // Enable TIM5 clock

}
