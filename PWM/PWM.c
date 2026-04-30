
#include <stddef.h>

#include "Pwm.h"
#include "../Timer/Timer_Private.h"   /* TimerType struct + base addresses */
#include "Bit_Math.h"

static TimerType* const PWM_ADREESES[6] = {
    NULL,
    NULL,
    TIM2,
    TIM3,
    TIM4,
    TIM5
};
#define CCR_REG(TIMER, CHANNEL)  *((volatile uint32 *) (&(TIMER->CCR[0]) + (CHANNEL - 1)))


void Pwm_Init(Tim_Instance_t TimerInstance,Tim_Channel_t Channel, Tim_Prescaler_t Prescaler, uint16 AutoReload) {

    TimerType * timer = PWM_ADREESES[TimerInstance];
    timer->CR[0] = 0; // Reset timer // counter disable
    timer->PSC = Prescaler;
    timer->ARR = AutoReload;
    timer->CNT = 0;
    if (Channel <= 2) {
        uint8 shift = (Channel - 1)*8;
        timer->CCMR[0] &= ~((uint32) 0xFF << shift);
        timer->CCMR[0] |= ((uint32) TIM_OC_PWM1_PRELOAD << shift);
    } else {
        uint8 shift = (Channel - 3) * 8;
        timer->CCMR[2] &= ~((uint32) 0xFF << shift);
        timer->CCMR[2] |= ((uint32) TIM_OC_PWM1_PRELOAD << shift);
    }
    SET_BIT(timer->CCER, (Channel - 1) * 4);
    CCR_REG(timer, Channel) = 0;

    SET_BIT(timer->CR[0], TIMER_CR1_ARPE);
    SET_BIT(timer->EGR, TIMER_EGR_UG);
    timer->SR = 0;
}

/**
 *  Fixed-point duty-cycle conversion (no float!)
 *  CCR = (DutyPercent * ARR) / 100
 */
void Pwm_SetDutyPercent(Tim_Instance_t TimerInstance, Tim_Channel_t Channel, uint8 DutyPercent) {
    TimerType * timer = PWM_ADREESES[TimerInstance];


    if (DutyPercent > 100) {
        DutyPercent = 100;
    }

    uint32 arr = timer->ARR;
    uint32 ccr = ((uint32) DutyPercent * arr) / 100UL;

    CCR_REG(timer, Channel) = ccr;
}

void Pwm_Start(Tim_Instance_t TimerInstance, Tim_Channel_t Channel) {
    TimerType * timer = PWM_ADREESES[TimerInstance];


    /* Make sure channel output is enabled */
    SET_BIT(timer->CCER, (Channel - 1) * 4);
    /* Start the counter */
    SET_BIT(timer->CR[0], TIMER_CR1_CEN);
}

void Pwm_Stop(Tim_Instance_t TimerInstance, Tim_Channel_t Channel) {
    TimerType * timer = PWM_ADREESES[TimerInstance];

    /* Disable channel output */
    CLR_BIT(timer->CCER, (Channel - 1) * 4);

    CLR_BIT(timer->CR[0], TIMER_CR1_CEN);
}
