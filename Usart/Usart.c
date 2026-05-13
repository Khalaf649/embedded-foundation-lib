//
// Created by Khalaf on 29/04/2026.
//
#include "stm32f401xe.h"
#include "Usart.h"
#include "BIT_MATH.h"
#include "../GPIO/GPIO.h"
#include "../RCC/RCC.h"



void Usart1_Init(void) {
    uint8 usart1_bus_id = 3;
    uint8 usart1_bit_pos = 4;

     // Calculate final ID: (BusID * 32) + BitPos
    uint8 usart1_peripheral_id = (usart1_bus_id * 32) + usart1_bit_pos;

    RCC_EnablePeripheral(usart1_peripheral_id);
    // 1. Define the pin locations for TX (PA9) and RX (PA10)
    GPIO_Pin_Location_t tx_pin = {GPIO_PORT_A, GPIO_PIN_9};
    GPIO_Pin_Location_t rx_pin = {GPIO_PORT_A, GPIO_PIN_10};

    GPIO_PinConfig_t usart_config;
    GPIO_PrepareConfig(&usart_config,
                       GPIO_MODE_AF,
                       GPIO_PULL_UP,
                       GPIO_SPEED_HIGH,
                       GPIO_OTYPE_PP);

    // 2. Initialize both pins using the handles
    GPIO_InitPin(&tx_pin, &usart_config);
    GPIO_InitPin(&rx_pin, &usart_config);

    GPIO_SetAlternateFunction(&tx_pin, GPIO_AF_7);
    GPIO_SetAlternateFunction(&rx_pin, GPIO_AF_7);

    // 3. Configure USART1 Frame (8 Data bits, 1 Stop bit, No Parity)
    CLR_BIT(USART1->CR1, USART_CR1_M_Pos); // 8-bit word length
    WRITE_BIT_FIELD(USART1->CR2, 0x11, 6, 2, 0x00);

    USART1->CR2 &= ~(USART_CR2_STOP_Msk); // 1-stop bit at the end
    CLR_BIT(USART1->CR1, USART_CR1_OVER8_Pos);  // 16 over sampling

    // 4. Hardcoded Baud Rate (9600 based on your clock tree setup)
    USART1->BRR = 0x683;

    // 5. Enable TX, RX, and the USART peripheral
    SET_BIT(USART1->CR1, USART_CR1_TE_Pos);
    SET_BIT(USART1->CR1, USART_CR1_RE_Pos);
    SET_BIT(USART1->CR1, USART_CR1_UE_Pos);
}

uint8 Usart1_TransmitByte(uint8 Byte) {
    if (USART1->SR & USART_SR_TXE_Msk) {
        USART1->DR = Byte;
        while (!(USART1->SR & USART_SR_TC_Msk));
        USART1->SR &= ~(USART_SR_TC_Msk); // Clearing TC bit
        return E_OK;
    }
    return E_NOT_OK;
}

void Usart1_TransmitString(const char *Str) {
    uint32 i = 0;
    uint8 transmitResult = -1;
    while (Str[i] != '\0') {
        transmitResult = Usart1_TransmitByte((uint8)Str[i]);
        if (transmitResult == E_OK) {
            i++;
        }
    }
}



uint8 Usart1_RecieveByte(void) {
    while (!(USART1->SR & USART_SR_RXNE_Msk));
    return USART1->DR;
}