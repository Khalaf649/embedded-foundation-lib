//
// Scheduler.c
// Hardware-based cooperative task scheduler.
//
// Architecture:
//   TIM4 fires every 1ms → ISR increments g_system_tick.
//   Scheduler_Run() (called in main super-loop) checks each registered
//   task and calls it if (now - last_tick) >= period_ms.
//
// Rules for task functions:
//   - MUST return quickly (no blocking, no polling loops).
//   - May set flags; heavier work is done by the FSM reading those flags.
//   - MUST NOT call Scheduler_RegisterTask from inside a task.
//

#include "Scheduler.h"
#include "../Timer/Timer.h"
#include "../Nvic/Nvic.h"   // ENTER_CRITICAL / EXIT_CRITICAL
#include <stddef.h>

#include "../Usart/Usart.h"

// =========================================================
// Private state
// =========================================================
static volatile uint32    g_system_tick                      = 0;
static Scheduler_Task_t   g_tasks[SCHEDULER_MAX_TASKS];
static uint8              g_task_count                       = 0;

// =========================================================
// ISR callback — fired by TIM4 every 1 ms
// =========================================================
static void Scheduler_TickCallback(void) {
    g_system_tick++;
}

// =========================================================
// Scheduler_Init
// =========================================================
void Scheduler_Init(void) {
    // Zero out the task table before anything else
    for (uint8 i = 0; i < SCHEDULER_MAX_TASKS; i++) {
        g_tasks[i].task       = NULL;
        g_tasks[i].period_ms  = 0;
        g_tasks[i].last_tick  = 0;
        g_tasks[i].enabled    = FALSE;
    }
    g_task_count = 0;

    // Start TIM4 at 1ms period — this also enables the NVIC IRQ
    // internally, so the tick starts running immediately.
    Timer_Init(TIM_INSTANCE_4, 15, 999);
    Timer_StartPeriodic(TIM_INSTANCE_4, 2, Scheduler_TickCallback);
}

// =========================================================
// Scheduler_RegisterTask
// =========================================================
uint8 Scheduler_RegisterTask(Scheduler_TaskFn_t task, uint32 period_ms) {
    if (task == NULL)                       return 0xFF;
    if (period_ms == 0)                     return 0xFF;
    if (g_task_count >= SCHEDULER_MAX_TASKS) return 0xFF;

    uint8 slot = g_task_count;

    // Snapshot the current tick so the first call happens exactly
    // one period from now (avoids a burst of calls at startup).
    ENTER_CRITICAL();
    g_tasks[slot].task      = task;
    g_tasks[slot].period_ms = period_ms;
    g_tasks[slot].last_tick = g_system_tick;  // start the period from now
    g_tasks[slot].enabled   = TRUE;
    g_task_count++;
    EXIT_CRITICAL();

    return slot;
}

// =========================================================
// Scheduler_Run  (call in while(1))
// =========================================================
void Scheduler_Run(void) {
    // Take a stable snapshot of the tick to avoid skew between tasks
    // if the ISR fires mid-loop.
    uint32 now;
    ENTER_CRITICAL();
    now = g_system_tick;
    EXIT_CRITICAL();

    for (uint8 i = 0; i < g_task_count; i++) {
        if (!g_tasks[i].enabled) continue;

        // Unsigned subtraction handles the uint32 wrap-around correctly.
        if ((now - g_tasks[i].last_tick) >= g_tasks[i].period_ms) {
            // Advance last_tick by the period (not by 'now') to prevent
            // drift accumulation over many cycles.
            g_tasks[i].last_tick += g_tasks[i].period_ms;

            // Dispatch
            g_tasks[i].task();
        }
    }
}

// =========================================================
// Scheduler_GetTick
// =========================================================
uint32 Scheduler_GetTick(void) {
    uint32 tick;
    ENTER_CRITICAL();
    tick = g_system_tick;
    EXIT_CRITICAL();
    return tick;
}

// =========================================================
// Scheduler_EnableTask / Scheduler_DisableTask
// =========================================================
void Scheduler_EnableTask(uint8 task_index) {
    if (task_index >= g_task_count) return;
    ENTER_CRITICAL();
    // Reset last_tick so it gets a fresh period from now
    g_tasks[task_index].last_tick = g_system_tick;
    g_tasks[task_index].enabled   = TRUE;
    EXIT_CRITICAL();
}

void Scheduler_DisableTask(uint8 task_index) {
    if (task_index >= g_task_count) return;
    g_tasks[task_index].enabled = FALSE;
}