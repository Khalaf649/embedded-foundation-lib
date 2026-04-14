//
// Created by Khalaf on 13/04/2026.
//

#include "GPIO.h"
#include "GPIO_private.h"
#include "BIT_MATH.h"
#include "RCC.h"
void GPIO_InitPin(const GPIO_Pin_Handle_t pin_h, const GPIO_Config_Handle_t config_h) {
    if (pin_h == NULL || config_h == NULL) return;

    /* 1. Get the base address of the port */
    GPIO_RegDef_t* GPIOx = GPIO_PORT_LOOKUP[pin_h->port];
    uint8_t pin = pin_h->pin_number;
     /* 2. Enable Clock for the Port (Integrated for now) */
    uint8 PeripheralId = (RCC_AHB1_BUS * 32) + (uint8)pin_h->port;
    RCC_EnablePeripheral(PeripheralId);

    /* 3. Configure Mode (2 bits per pin) */
    WRITE_BIT_FIELD(GPIOx->MODER, 0x03, pin, 2, config_h->mode);

    /* 4. Configure Pull-up/down (2 bits per pin) */
    WRITE_BIT_FIELD(GPIOx->PUPDR, 0x03, pin, 2, config_h->pull);

    /* 5. Configure Output Type (1 bit per pin) */
    WRITE_BIT_FIELD(GPIOx->OTYPER, 0x01, pin, 1, config_h->otype);

    /* 6. Configure Output Speed (2 bits per pin) */
    WRITE_BIT_FIELD(GPIOx->OSPEEDR, 0x03, pin, 2, config_h->speed);
}
void GPIO_PrepareConfig(P_void config_out, GPIO_Mode_t mode, GPIO_Pull_t pull, GPIO_Speed_t speed, GPIO_OType_t otype) {
    if (config_out == NULL) return;

    GPIO_Config_Handle_t cfg = (GPIO_Config_Handle_t)config_out;
    cfg->mode  = mode;
    cfg->pull  = pull;
    cfg->speed = speed;
    cfg->otype = otype;
}

void GPIO_WritePin(const GPIO_Pin_Handle_t pin_h, GPIO_PinState_t state) {
    if (pin_h == NULL) return;

    GPIO_RegDef_t* GPIOx = GPIO_PORT_LOOKUP[pin_h->port];


    WRITE_BIT_FIELD(GPIOx->ODR, 0x01, pin_h->pin_number, 1, state);


}

GPIO_PinState_t GPIO_ReadPin(const GPIO_Pin_Handle_t pin_h) {
    if (pin_h == NULL) return GPIO_LOW;

    GPIO_RegDef_t* GPIOx = GPIO_PORT_LOOKUP[pin_h->port];

    /* We read from the IDR (Input Data Register) */
    /* Return the specific bit corresponding to the pin number */
    return GET_BIT(GPIOx->IDR, pin_h->pin_number);
}

void GPIO_TogglePin(const GPIO_Pin_Handle_t pin_h) {
    if (pin_h == NULL) return;

    GPIO_RegDef_t* GPIOx = GPIO_PORT_LOOKUP[pin_h->port];

    /* Using your TOG_BIT macro */
    TOG_BIT(GPIOx->ODR, pin_h->pin_number);
}