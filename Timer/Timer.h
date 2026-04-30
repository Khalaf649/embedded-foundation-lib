//
// Created by Khalaf on 30/04/2026.
//

#ifndef TIMER_H
#define TIMER_H
#include <STD_TYPES.h>
#include "Timer.h"

typedef enum {
    /* OCxM[2:0] = 011 (Toggle on match), OCxPE = 0 */
    TIM_OC_TOGGLE       = 0x30U,

    /* OCxM[2:0] = 110 (PWM Mode 1), OCxPE = 1 (Preload enable) */
    TIM_OC_PWM1_PRELOAD = 0x68U
} Tim_OutputCompareMode_t;

typedef enum {
    TIM_INSTANCE_2 = 2U,
    TIM_INSTANCE_3 = 3U,
    TIM_INSTANCE_4 = 4U,
    TIM_INSTANCE_5 = 5U
} Tim_Instance_t;


typedef enum {
    TIM_CHANNEL_1 = 1U,
    TIM_CHANNEL_2 = 2U,
    TIM_CHANNEL_3 = 3U,
    TIM_CHANNEL_4 = 4U
} Tim_Channel_t;
typedef enum {
    /* 16 MHz / (0 + 1) = 16 MHz (Maximum speed) */
    TIM_PRESCALER_DIV_1     = 0U,

    /* 16 MHz / (15 + 1) = 1 MHz Timer Clock (1 tick = 1 microsecond) */
    TIM_PRESCALER_1US_TICK  = 15U,

    /* 16 MHz / (15999 + 1) = 1 kHz Timer Clock (1 tick = 1 millisecond) */
    TIM_PRESCALER_1MS_TICK  = 15999U

} Tim_Prescaler_t;
typedef void (*TimerCallback_t)(void);
void Timer_Init(Tim_Instance_t TimerInstance, Tim_Prescaler_t Prescaler, uint16 AutoReload);
void Timer_Start(Tim_Instance_t TimerInstance);

void Timer_Stop(Tim_Instance_t TimerInstance);

void Timer_DelayMs(Tim_Instance_t TimerInstance, uint32 DelayMs);
void Timer_ConfigChannel(Tim_Instance_t TimerInstance, Tim_Channel_t Channel,Tim_Prescaler_t Prescaler
                         , Tim_OutputCompareMode_t Mode, uint16 Period);
void Timer_DelayMsAsync(Tim_Instance_t TimerInstance, uint32 DelayMs, TimerCallback_t Callback);
void Timer_SetCompareValue(Tim_Instance_t TimerInstance, Tim_Channel_t Channel, uint32 CompareValue);
#endif //TIMER_H
