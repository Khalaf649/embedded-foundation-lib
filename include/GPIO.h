//
// Created by Khalaf on 13/04/2026.
//

#ifndef GPIO_H
#define GPIO_H

#include "STD_TYPES.h"

/* --- Port Definitions --- */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E
} GPIO_Port_t;

/* --- Pin ID Definitions --- */
typedef enum {
    GPIO_PIN_0 = 0, GPIO_PIN_1,  GPIO_PIN_2,  GPIO_PIN_3,
    GPIO_PIN_4,     GPIO_PIN_5,  GPIO_PIN_6,  GPIO_PIN_7,
    GPIO_PIN_8,     GPIO_PIN_9,  GPIO_PIN_10, GPIO_PIN_11,
    GPIO_PIN_12,    GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
} GPIO_Pin_t;

/* --- Pin Configuration Enums --- */
typedef enum {
    GPIO_MODE_OUTPUT = 0x00,
    GPIO_MODE_INPUT  = 0x01,
    GPIO_MODE_AF     = 0x02,
    GPIO_MODE_ANALOG = 0x03,
    GPIO_MODE_IT_RISING,   /* Trigger for Emergency Reset / Door Bell */
    GPIO_MODE_IT_FALLING,
    GPIO_MODE_IT_BOTH
} GPIO_Mode_t;

typedef enum {
    GPIO_PULL_NONE = 0x00,
    GPIO_PULL_UP   = 0x01,
    GPIO_PULL_DOWN = 0x02
} GPIO_Pull_t;

typedef enum {
    GPIO_SPEED_LOW = 0x00,
    GPIO_SPEED_MEDIUM,
    GPIO_SPEED_HIGH,
    GPIO_SPEED_VHIGH
} GPIO_Speed_t;

typedef enum {
    GPIO_OTYPE_PP = 0x00,
    GPIO_OTYPE_OD = 0x01
} GPIO_OType_t;

typedef enum {
    GPIO_LOW = 0,
    GPIO_HIGH = 1
} GPIO_PinState_t;

/* --- Structure Definitions --- */

typedef struct {
    GPIO_Port_t port;
    GPIO_Pin_t  pin_number;
} GPIO_Pin_Location_t;

typedef struct {
    GPIO_Mode_t  mode;
    GPIO_Pull_t  pull;
    GPIO_Speed_t speed;
    GPIO_OType_t otype;
} GPIO_PinConfig_t;

/* --- Pointer Typedefs (Handles) --- */
typedef GPIO_Pin_Location_t* GPIO_Pin_Handle_t;
typedef GPIO_PinConfig_t* GPIO_Config_Handle_t;

/* --- CORE APIs --- */


void GPIO_InitPin(const GPIO_Pin_Handle_t pin_h, const GPIO_Config_Handle_t config_h);


void GPIO_WritePin(const GPIO_Pin_Handle_t pin_h, GPIO_PinState_t state);


GPIO_PinState_t GPIO_ReadPin(const GPIO_Pin_Handle_t pin_h);


void GPIO_TogglePin(const GPIO_Pin_Handle_t pin_h);


void GPIO_PrepareConfig(P_void config_out, GPIO_Mode_t mode, GPIO_Pull_t pull, GPIO_Speed_t speed, GPIO_OType_t otype);

#endif //GPIO_H