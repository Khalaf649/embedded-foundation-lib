/*
 * Dispatcher.h — Master-only hallway call dispatcher
 *
 * Corner cases handled:
 *  - Comm fault: all calls to Master; Slave enters INDEPENDENT_MODE
 *  - Both elevators in EMERGENCY: calls queued, served on recovery
 *  - Duplicate hallway calls deduplicated (pending call table)
 *  - Elevator enters EMERGENCY mid-travel: re-dispatch affected call
 *  - Slave restores comms: resume normal dispatch, no retroactive moves
 *  - Invalid call_floor ([0] or [>4]): logged and discarded
 *  - SPI snapshot age-checked (>150ms = stale, treat as comm fault)
 *
 * Volatile flags used:
 *  g_HallwayUpRequests[], g_HallwayDownRequests[] (Button.h) — consumed here
 *  g_rx_packet (Ipc.h) — read for slave state snapshot
 *  g_tx_packet (Ipc.h) — written for slave target assignment
 *  g_comm_fault (Ipc.h) — read for comm fault detection
 *
 * Driver API assumptions:
 *  ElevatorFSM_AssignTarget(), ElevatorFSM_Get*() accessors
 *  ENTER/EXIT_CRITICAL(), Usart1_TransmitString()
 *  Scheduler_GetTick() for SPI age checking
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#ifdef BUILD_AS_MASTER

#include "../include/STD_TYPES.h"
#include "../ElevatorFSM/ElevatorFSM.h"

/* ================================================================
 * Constants
 * ================================================================ */
#define DISPATCHER_PERIOD_MS        10U     /* Scheduler calls every 10ms   */
#define DISPATCHER_SPI_MAX_AGE_MS   150U    /* Reject stale SPI snapshots   */
#define MAX_PENDING_CALLS           12U     /* 6 hallway buttons × 2 dirs   */

/* ================================================================
 * Call direction — hallway UP / DOWN buttons
 * ================================================================ */
typedef enum {
    CALL_DIR_UP   = 0,
    CALL_DIR_DOWN = 1
} CallDirection_t;

/* ================================================================
 * Pending call entry
 * ================================================================ */
typedef struct {
    Floor_t         floor;          /* Call floor [FLOOR_1..FLOOR_4]     */
    CallDirection_t direction;      /* UP or DOWN                       */
    boolean         active;         /* TRUE if call is pending/assigned  */
    boolean         assigned;       /* TRUE if dispatched to an elevator */
    uint8           assigned_to;    /* 0 = Elevator A, 1 = Elevator B   */
} PendingCall_t;

/* ================================================================
 * Dispatch score values (higher = better candidate)
 * ================================================================ */
#define DISPATCH_SCORE_IMMEDIATE        100U
#define DISPATCH_SCORE_PERFECT_MATCH     80U
#define DISPATCH_SCORE_PASSED_MATCH      40U
#define DISPATCH_SCORE_IDLE_NEAREST      20U
#define DISPATCH_SCORE_OPPOSITE_DIR       0U   /* Excluded from consideration */
#define DISPATCH_SCORE_UNAVAILABLE        0U

/* ================================================================
 * Public API
 * ================================================================ */

/** Initialize dispatcher. Call once after FSMs are initialized.
 *  fsm_a = local elevator FSM pointer
 *  No fsm_b pointer needed — slave state read from g_rx_packet. */
void Dispatcher_Init(ElevatorFSM_t *fsm_a);

/** Process pending hallway calls and assign to best elevator.
 *  Called every DISPATCHER_PERIOD_MS by scheduler. */
void Dispatcher_Tick(void);

/** Force-assign a floor to Elevator A (used during comm fault). */
void Dispatcher_AssignToA(Floor_t floor);

/** Write slave target into g_tx_packet for IPC transmission. */
void Dispatcher_PublishSlaveTarget(Floor_t floor);

/** Re-dispatch any calls that were assigned to an elevator that
 *  has entered EMERGENCY or gone offline. */
void Dispatcher_ReassignCalls(uint8 elevator_id);

#endif /* BUILD_AS_MASTER */
#endif /* DISPATCHER_H */
