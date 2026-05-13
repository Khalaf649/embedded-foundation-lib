//
// Created by Khalaf on 13/05/2026.
//

#include "Scheduler.h"
#include "../Timer/Timer.h"

volatile uint32 g_system_tick = 0;

static void Scheduler_TickCallback(void) {
    g_system_tick++;
}

void Scheduler_Init(void) {

    Timer_StartPeriodic(TIM_INSTANCE_4, 1, Scheduler_TickCallback);
}

uint32 Scheduler_GetTick(void) {
    return g_system_tick;
}