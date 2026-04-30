//
// Created by Khalaf on 30/04/2026.
//

#include "Timer.h"

#include <BIT_MATH.h>
#include <stddef.h>
#include <stm32f4xx.h>

#include "Timer_Private.h"
#include "../Nvic/Nvic.h"
static TimerType* const Timers[6] = {
    NULL,
    NULL,
    TIM2,
    TIM3,
    TIM4,
    TIM5
};
static TimerCallback_t Tim_AsyncCallbacks[6] = {NULL};
static uint8 Timer_NvicIrq[6] = {0 ,0,28, 29, 30, 50};

void Timer_Init(Tim_Instance_t TimerInstance, Tim_Prescaler_t Prescaler, uint16 AutoReload) {
    TimerType * timer = Timers[TimerInstance];
    timer->CR[0] = 0; // Reset timer // counter disable
    timer->PSC = Prescaler;
    timer->ARR = AutoReload;
    timer->CNT = 0;
    SET_BIT(timer->EGR, TIMER_EGR_UG); // Force update to load PSC & ARR
    timer->SR = 0; // Clear the update flag that UG set

}
void Timer_Start(Tim_Instance_t TimerInstance) {
    TimerType * timer = Timers[TimerInstance];
    SET_BIT(timer->CR[0], TIMER_CR1_CEN);


}
void Timer_Stop(Tim_Instance_t TimerInstance) {
    TimerType * timer = Timers[TimerInstance];
    CLR_BIT(timer->CR[0], TIMER_CR1_CEN);

}
void Timer_DelayMs(Tim_Instance_t TimerInstance, uint32 DelayMs){
    TimerType * timer = Timers[TimerInstance];
    timer->CR[0] = 0; // Stop & reset
    timer->PSC = 15999U / 3;
    timer->ARR = (uint16) (DelayMs - 1);
    timer->CNT = 0;
    SET_BIT(timer->EGR, TIMER_EGR_UG); // Load shadow registers
    timer->SR = 0; // Clear UIF caused by UG
    SET_BIT(timer->CR[0], TIMER_CR1_OPM); // One-pulse mode
    SET_BIT(timer->CR[0], TIMER_CR1_CEN); // Start counting
    while (!GET_BIT(timer->SR, TIMER_SR_UIF)) {
        // Poll – CPU is blocked here
    }
    timer->SR = 0; // Clear UIF
    CLEAR_BIT(timer->CR[0], TIMER_CR1_CEN); // Stop counter


}
void Timer_DelayMsAsync(Tim_Instance_t TimerInstance, uint32 DelayMs, TimerCallback_t Callback) {
    TimerType * timer = Timers[TimerInstance];
    uint8 irqNum = Timer_NvicIrq[TimerInstance];

    Tim_AsyncCallbacks[TimerInstance] = Callback;

    timer->CR[0] = 0; // Stop & reset
    timer->PSC = 15999U / 3;
    timer->ARR = (uint16) (DelayMs - 1);
    timer->CNT = 0;

    SET_BIT(timer->EGR, TIMER_EGR_UG); // Load shadow registers
    timer->SR = 0; // Clear UIF caused by UG

    SET_BIT(timer->CR[0], TIMER_CR1_OPM); // One-pulse mode

    SET_BIT(timer->DIER, TIMER_DIER_UIE); // Enable update interrupt
    Nvic_EnableInterrupt(irqNum);

    SET_BIT(timer->CR[0], TIMER_CR1_CEN); // Start counting
}
void Timer_ConfigChannel(Tim_Instance_t TimerInstance, Tim_Channel_t Channel, Tim_OutputCompareMode_t Mode) {
    /* Get hardware pointer */
    TimerType * timer = Timers[TimerInstance];
    if (timer == NULL) return;

    /* Shift from CH1-CH4 (1-4) to Array Index (0-3) */
    Channel--;

    /* Write Mode: Ch 0,1 -> CCMR[0] | Ch 2,3 -> CCMR[1] */
    WRITE_BIT_FIELD(timer->CCMR[Channel / 2], 0xFFU, (Channel % 2), 8U, Mode);

    /* Reset compare value */
    timer->CCR[Channel] = 0;

    /* Enable channel output pin (CCxE bits are 4 apart) */
    SET_BIT(timer->CCER, Channel * 4);

    /* Force shadow registers to load */
    SET_BIT(timer->EGR, TIMER_EGR_UG);

    /* Clear false update flag caused by EGR */
    timer->SR = 0;

    /* Start the timer */
    SET_BIT(timer->CR[0], TIMER_CR1_CEN);
}
void Timer_SetCompareValue(Tim_Instance_t TimerInstance, Tim_Channel_t Channel, uint32 CompareValue) {
    /* Get hardware pointer */
    TimerType * timer = Timers[TimerInstance];
    if (timer == NULL) return;

    /* Shift from CH1-CH4 (1-4) to Array Index (0-3) */
    Channel--;

    /* Mathematically direct assignment (zero branch instructions) */
    timer->CCR[Channel] = CompareValue;
}
static void Timer_HandleIrq(Tim_Instance_t TimerInstance) {
    TimerType * timer = Timers[TimerInstance];


    if (GET_BIT(timer->SR, TIMER_SR_UIF)) {
        timer->SR = 0; // Clear UIF
        CLR_BIT(timer->DIER, TIMER_DIER_UIE); // Disable further IRQs
        CLR_BIT(timer->CR[0], TIMER_CR1_CEN); // Stop counter

        if (Tim_AsyncCallbacks[TimerInstance] != 0) {
            Tim_AsyncCallbacks[TimerInstance]();
        }
    }
}




void TIM2_IRQHandler(void) {
    Timer_HandleIrq(0);
}
void TIM3_IRQHandler(void) {
    Timer_HandleIrq(1);
}
void TIM4_IRQHandler(void) {
    Timer_HandleIrq(2);
}
void TIM5_IRQHandler(void) {
    Timer_HandleIrq(3);
}
