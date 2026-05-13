//
// Scheduler.h
// Hardware-based cooperative task scheduler (TIM4 @ 1ms tick)
// Supports up to SCHEDULER_MAX_TASKS periodic tasks at independent periods.
//

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "STD_TYPES.h"

// =========================================================
// Configuration
// =========================================================
#define SCHEDULER_MAX_TASKS   8U    // Maximum number of registered tasks

// =========================================================
// Types
// =========================================================
typedef void (*Scheduler_TaskFn_t)(void);

typedef struct {
    Scheduler_TaskFn_t task;        // Function pointer to the task
    uint32             period_ms;   // How often to run (milliseconds)
    uint32             last_tick;   // Tick value when last executed
    boolean            enabled;     // Task can be suspended without removal
} Scheduler_Task_t;

// =========================================================
// API
// =========================================================

/**
 * Scheduler_Init
 * Configures TIM4 to fire every 1ms and increments the internal
 * tick counter from the ISR. Call once before Scheduler_RegisterTask.
 */
void Scheduler_Init(void);

/**
 * Scheduler_RegisterTask
 * Adds a periodic task to the scheduler table.
 *
 * @param task       Function to call (must return quickly — no blocking)
 * @param period_ms  Period in milliseconds (must be >= 1)
 * @return           Task slot index (0..SCHEDULER_MAX_TASKS-1), or 0xFF on failure
 */
uint8 Scheduler_RegisterTask(Scheduler_TaskFn_t task, uint32 period_ms);

/**
 * Scheduler_Run
 * Call this in the main super-loop (while(1)).
 * Checks all registered tasks and dispatches any that are due.
 * Non-blocking — returns immediately if no tasks are ready.
 */
void Scheduler_Run(void);

/**
 * Scheduler_GetTick
 * Returns the raw millisecond tick counter.
 * Wraps at ~49 days (UINT32_MAX ms).
 */
uint32 Scheduler_GetTick(void);

/**
 * Scheduler_EnableTask / Scheduler_DisableTask
 * Suspend or resume a task by its slot index without removing it.
 */
void Scheduler_EnableTask(uint8 task_index);
void Scheduler_DisableTask(uint8 task_index);

#endif // SCHEDULER_H