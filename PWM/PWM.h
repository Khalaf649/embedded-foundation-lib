//
// Created by Khalaf on 30/04/2026.
//

#ifndef PWM_H
#define PWM_H
#include "STD_TYPES.h"
#include "../Timer/Timer.h"
void Pwm_Init(Tim_Instance_t TimerInstance,Tim_Channel_t Channel, Tim_Prescaler_t Prescaler, uint16 AutoReload);

void Pwm_SetDutyPercent(Tim_Instance_t TimerInstance, Tim_Channel_t Channel, uint8 DutyPercent);

/**
 * @brief  Start PWM output on the given channel.
 */
void Pwm_Start(Tim_Instance_t TimerInstance, Tim_Channel_t Channel);

/**
 * @brief  Stop PWM output on the given channel.
 */
void Pwm_Stop(Tim_Instance_t TimerInstance, Tim_Channel_t Channel);

#endif //PWM_H
