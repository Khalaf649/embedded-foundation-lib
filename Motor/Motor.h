#ifndef STM32_TEMPLATE_MOTOR_H
#define STM32_TEMPLATE_MOTOR_H

#include "STD_TYPES.h"
#include "../GPIO/GPIO.h"
#include "../PWM/PWM.h"
#include "../USART/USART.h"
#include "../ElevatorFSM/ElevatorFSM.h"

/* --- Hardware Mapping --- */
#define MOTOR_TIMER      TIM_INSTANCE_3
#define MOTOR_CHANNEL    TIM_CHANNEL_3
#define MOTOR_PORT       GPIO_PORT_B
#define MOTOR_PIN        GPIO_PIN_0



/* --- APIs --- */
void Motor_Init(void);
void Motor_SetSpeed(Motor_Speed_t speed_lvl);
Motor_Speed_t Motor_GetSpeed(void);

#endif //STM32_TEMPLATE_MOTOR_H