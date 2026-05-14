//
// Dispatcher.h
// Master-only hallway call dispatcher.
//
// Compiled only when BUILD_AS_MASTER is defined.
// The Dispatcher runs the mandated priority algorithm every time a
// hallway button is pressed and assigns the call to the optimal elevator.
//
// Dispatch priority (as specified):
//   1. Comm Fault       — Master handles all calls; Slave enters independent mode.
//   2. Immediate        — Elevator already at the called floor and IDLE.
//   3. Perfect Match    — Elevator moving towards floor in the same direction.
//   4. Passed Match     — Same direction but elevator has already passed floor.
//   5. Idle             — Nearest idle elevator (no directional match).
//   6. Opposite Dir     — Moving away; lowest priority, defer until path complete.
//

#ifndef DISPATCHER_H
#define DISPATCHER_H

#ifdef BUILD_AS_MASTER

#include "../include/Std_Types.h"
#include "../ElevatorFSM/ElevatorFSM.h"

/* ================================================================
 * Call direction — used for hallway UP / DOWN buttons
 * ================================================================ */
typedef enum {
    CALL_DIR_UP   = 0,
    CALL_DIR_DOWN = 1
} CallDirection_t;

/* ================================================================
 * Score values for the priority algorithm
 * Higher score = better candidate.
 * ================================================================ */
#define DISPATCH_SCORE_IMMEDIATE     100
#define DISPATCH_SCORE_PERFECT_MATCH  80
#define DISPATCH_SCORE_PASSED_MATCH   40
#define DISPATCH_SCORE_IDLE           20
#define DISPATCH_SCORE_OPPOSITE_DIR    5
#define DISPATCH_SCORE_NO_MATCH        0

/* ================================================================
 * Public API
 * ================================================================ */

/* Call once during App_Init (after FSMs are initialised) */
void Dispatcher_Init(ElevatorFSM_t *fsm_a, ElevatorFSM_t *fsm_b);

/* Called every DISPATCHER_POLL_MS by the scheduler.
 * Scans g_HallwayUpRequests / g_HallwayDownRequests and dispatches
 * each pending call to the best elevator. */
void Dispatcher_Tick(void);

/* Force-assign a floor to Elevator A (local) — used during comm fault */
void Dispatcher_AssignToA(Floor_t floor);

/* Write the current assignment for Elevator B into g_tx_packet so the
 * Slave receives it via IPC.  Called from within Dispatcher_Tick. */
void Dispatcher_PublishSlaveTarget(Floor_t floor);

#endif /* BUILD_AS_MASTER */
#endif /* DISPATCHER_H */
