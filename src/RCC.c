//
// Created by Khalaf on 14/04/2026.
//
#include "RCC.h"

#include "BIT_MATH.h"
#include "RCC_private.h"
static volatile uint32* const RCC_REGS[] = {
    &(RCC->AHB1ENR), /* Index 0 */
    &(RCC->AHB2ENR), /* Index 1 */
    &(RCC->APB1ENR), /* Index 2 */
    &(RCC->APB2ENR)  /* Index 3 */
};
void RCC_EnablePeripheral(uint8 PeripheralId) {
    uint8 BusId  = PeripheralId / 32;
    uint8 BitPos = PeripheralId % 32;

    if (BusId < 4) {
        SET_BIT(*(RCC_REGS[BusId]), BitPos);
    }
}
void Rcc_Init(void) {
    SET_BIT(RCC->CR, 0);
}
