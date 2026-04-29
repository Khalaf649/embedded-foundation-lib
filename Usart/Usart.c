//
// Created by Khalaf on 29/04/2026.
//
#include "stm32f401xe.h"
#include "Usart.h"
#include "../GPIO/GPIO.h"
#include "Std_Types.h"
void Usart1_Init(void) {
    // 1. Define the pin locations for TX (PA9) and RX (PA10)
    GPIO_Pin_Location_t tx_pin = {GPIO_PORT_A, GPIO_PIN_9};
    GPIO_Pin_Location_t rx_pin = {GPIO_PORT_A, GPIO_PIN_10};

    GPIO_PinConfig_t usart_config;
    GPIO_PrepareConfig(&usart_config,
                       GPIO_MODE_AF,
                       GPIO_PULL_UP,
                       GPIO_SPEED_HIGH,
                       GPIO_OTYPE_PP);

    // 3. Initialize both pins using the handles
    GPIO_InitPin(&tx_pin, &usart_config);
    GPIO_InitPin(&rx_pin, &usart_config);

    GPIO_SetAlternateFunction(&tx_pin,GPIO_AF_7);
    GPIO_SetAlternateFunction(&rx_pin, GPIO_AF_7);


    USART1->CR1 &= ~(1 << USART_CR1_M_Pos); // 8-bit word length

    USART1->CR2 &= ~(USART_CR2_STOP_Msk); // 1-stop bit at the end

    USART1->CR1 &= ~(1 << USART_CR1_OVER8_Pos); // 16 over sampling

    USART1->BRR = 0x683; // Baud Rate 9600

    /* Enable Transmission block */
    USART1->CR1 |= (1 << USART_CR1_TE_Pos);

    // /* Enable Receive block */
    USART1->CR1 |= (1 << USART_CR1_RE_Pos);

    /* Enable USART1 */
    USART1->CR1 |= (1 << USART_CR1_UE_Pos);
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
        transmitResult = Usart1_TransmitByte(Str[i]);
        if (transmitResult == E_OK) {
            i++;
        }
    }
}

uint8 Usart1_RecieveByte(void) {
    while (!(USART1->SR & USART_SR_RXNE_Msk));
    return USART1->DR;
}