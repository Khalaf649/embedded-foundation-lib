//
// Created by Khalaf on 30/04/2026.
//
#include "Fan.h"
#include "../Timer/Timer_Private.h"
void Fan_Init(const Fan_Config_t* fan_cfg) {
    /* We use 1kHz: Prescaler 15 (1MHz tick), ARR 1000. */
    Pwm_Init(fan_cfg->timer_instance, fan_cfg->channel, TIM_PRESCALER_1US_TICK, 100);

    /* Ensure the fan starts in the IDLE state (0% speed)[cite: 39]. */
    Fan_SetSpeed(fan_cfg, 0);
    Pwm_Start(fan_cfg->timer_instance, fan_cfg->channel); // ✅ ADD THIS

}


void Fan_SetSpeed(const Fan_Config_t* fan_cfg, uint8 duty_cycle_percent) {
    /* Purely modular: Just pass the call to your validated Timer driver */
    Pwm_SetDutyPercent(fan_cfg->timer_instance, fan_cfg->channel, duty_cycle_percent);
}