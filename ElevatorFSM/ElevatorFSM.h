/*
 * ElevatorFSM.h
 * Finite State Machine for a single elevator cabin.
 *
 * ═══════════════════════════════════════════════════════════════════
 * CORNER CASES EXPLICITLY HANDLED
 * ═══════════════════════════════════════════════════════════════════
 *  - Emergency pre-empts ANY state (highest NVIC priority flag)
 *  - Emergency only clears on explicit reset (g_Emergency_Flag cleared
 *    externally, then FSM transitions EMERGENCY → IDLE)
 *  - INDEPENDENT_MODE entered on SPI comm fault; elevator serves only
 *    its own cabin button queue; exits when comm restores
 *  - Floor values clamped to [FLOOR_1..FLOOR_4] before ANY UART print
 *    to prevent the "Floor 9" bug from raw sensor/EXTI pin numbers
 *  - Duplicate cabin requests are deduplicated (queue checked before insert)
 *  - PWM ramp driven by timer-based state machine, never blocking delay
 *  - current_floor updated atomically from volatile sensor flag
 *
 * ═══════════════════════════════════════════════════════════════════
 * VOLATILE FLAGS DECLARED / USED
 * ═══════════════════════════════════════════════════════════════════
 *  - g_CabinRequests[5]       (Button.h) — read in FSM_CollectCabinRequests
 *  - g_Current_Floor_Sensor   (Button.h) — read in FSM_Tick for floor update
 *  - g_Emergency_Flag         (Button.h) — read in FSM_Tick, highest priority
 *  - g_tx_packet              (Ipc.h)    — written in FSM_PublishToIpc
 *  - g_rx_packet              (Ipc.h)    — read on Slave for dispatcher target
 *  - g_comm_fault             (Ipc.h)    — read for INDEPENDENT_MODE transition
 *
 * ═══════════════════════════════════════════════════════════════════
 * DRIVER API ASSUMPTIONS
 * ═══════════════════════════════════════════════════════════════════
 *  - Motor_SetSpeed(Motor_Speed_t) sets PWM duty immediately
 *  - Motor_GetSpeed() returns current Motor_Speed_t
 *  - ENTER_CRITICAL() / EXIT_CRITICAL() disable/enable global IRQs
 *  - Usart1_TransmitString(const char*) is blocking (polling OK for UART)
 */

#ifndef ELEVATOR_FSM_H
#define ELEVATOR_FSM_H

#include "../include/STD_TYPES.h"

/* ================================================================
 * Constants
 * ================================================================ */
#define NUM_FLOORS              4U
#define ELEVATOR_DOORS_OPEN_MS  3000U   /* Door dwell time: 3 seconds         */
#define ELEVATOR_RAMP_STEP_MS   50U     /* PWM ramp step interval             */
#define ELEVATOR_RAMP_STEPS     10U     /* ~500ms total ramp (10 × 50ms)      */
#define ELEVATOR_FSM_PERIOD_MS  10U     /* Scheduler calls FSM every 10 ms    */
#define MAX_QUEUE_SIZE          NUM_FLOORS

/* ================================================================
 * Floor identifiers (1-indexed for IPC/UART; 0 = no target)
 * Internal logic uses 0-indexed (floor - 1) where arithmetic is needed
 * ================================================================ */
typedef enum {
    FLOOR_NONE = 0,
    FLOOR_1    = 1,
    FLOOR_2    = 2,
    FLOOR_3    = 3,
    FLOOR_4    = 4
} Floor_t;

/* ================================================================
 * Elevator operating states
 * ================================================================ */
typedef enum {
    LIFT_STATE_IDLE             = 0,
    LIFT_STATE_MOVING_UP        = 1,
    LIFT_STATE_MOVING_DOWN      = 2,
    LIFT_STATE_DOORS_OPEN       = 3,
    LIFT_STATE_EMERGENCY        = 4,
    LIFT_STATE_INDEPENDENT      = 5
} LiftState_t;

/* ================================================================
 * Motor speed levels — values ARE the PWM duty-cycle percent
 * ================================================================ */
typedef enum {
    MOTOR_REST       = 0,
    MOTOR_LOW_SPEED  = 20,
    MOTOR_HIGH_SPEED = 100
} Motor_Speed_t;

/* ================================================================
 * Emergency flag for IPC packet
 * ================================================================ */
typedef enum {
    EMERGENCY_NORMAL = 0,
    EMERGENCY_STOP   = 1
} Emergency_t;

/* ================================================================
 * Cabin button bitmask encoding for IPC packet
 * Bit N-1 set means floor N is requested (bit 0 = floor 1)
 * ================================================================ */
#define CABIN_BTN_NONE   (0x00U)
#define CABIN_BTN_FLOOR1 (1U << 0)
#define CABIN_BTN_FLOOR2 (1U << 1)
#define CABIN_BTN_FLOOR3 (1U << 2)
#define CABIN_BTN_FLOOR4 (1U << 3)

/* ================================================================
 * PWM Ramp direction
 * ================================================================ */
typedef enum {
    RAMP_NONE     = 0,   /* No ramp in progress                        */
    RAMP_UP       = 1,   /* Accelerating: 0% → 20% → 100%             */
    RAMP_DOWN     = 2    /* Decelerating: 100% → 20% → 0%             */
} RampDirection_t;

/* ================================================================
 * PWM Ramp state machine
 * ================================================================ */
typedef struct {
    RampDirection_t direction;      /* Current ramp direction           */
    uint8           step_count;     /* Steps elapsed in current ramp    */
    uint8           target_duty;    /* Final duty cycle to reach        */
    uint8           current_duty;   /* Current duty cycle percentage    */
} RampState_t;

/* ================================================================
 * FSM context — one instance per board
 * ================================================================ */
typedef struct {
    LiftState_t     state;                  /* Current FSM state                */
    LiftState_t     pre_independent_state;  /* State before INDEPENDENT_MODE    */
    Floor_t         current_floor;          /* Last confirmed floor from sensor */
    Floor_t         target_floor;           /* Active destination               */
    Floor_t         queue[MAX_QUEUE_SIZE];  /* Cabin request queue              */
    uint8           queue_size;             /* Number of floors in queue        */
    uint32          door_timer_ms;          /* Accumulator for door open time   */
    boolean         emergency_active;       /* Latched emergency flag           */
    boolean         independent_mode;       /* TRUE when in INDEPENDENT_MODE    */
    RampState_t     ramp;                   /* PWM speed ramp state machine     */
} ElevatorFSM_t;

/* ================================================================
 * Public API
 * ================================================================ */

/** Initialize FSM to IDLE at floor 1. Call once after Motor_Init. */
void ElevatorFSM_Init(ElevatorFSM_t *fsm);

/** Run one FSM tick. Call every ELEVATOR_FSM_PERIOD_MS via scheduler.
 *  Reads global button/sensor/IPC flags; drives motor + IPC tx. */
void ElevatorFSM_Tick(ElevatorFSM_t *fsm);

/** Assign a target floor. Called by Dispatcher (Master) or IPC rx (Slave).
 *  No-op if emergency active or floor out of range. */
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor);

/** Advance PWM ramp state machine one step. Call every ELEVATOR_RAMP_STEP_MS. */
void ElevatorFSM_RampTick(ElevatorFSM_t *fsm);

/** Reset from emergency state. Only call after emergency condition cleared. */
void ElevatorFSM_ResetEmergency(ElevatorFSM_t *fsm);

/* ── Read-only accessors ── */
LiftState_t ElevatorFSM_GetState(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetCurrentFloor(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetTargetFloor(const ElevatorFSM_t *fsm);
boolean     ElevatorFSM_IsEmergency(const ElevatorFSM_t *fsm);
boolean     ElevatorFSM_IsIndependent(const ElevatorFSM_t *fsm);

#endif /* ELEVATOR_FSM_H */
