//
// Created by Khalaf on 14/05/2026.
//

#ifndef STM32_TEMPLATE_ELEVATORFSM_H
#define STM32_TEMPLATE_ELEVATORFSM_H
#include "../include/Std_Types.h"

/* ================================================================
 * Shared enum types
 * These are placed here so Dispatcher and IPC can include one header
 * instead of duplicating definitions.
 * ================================================================ */

/* Floor identifiers — FLOOR_NONE means "no target assigned" */
typedef enum {
    FLOOR_NONE = 0,
    FLOOR_1    = 1,
    FLOOR_2    = 2,
    FLOOR_3    = 3,
    FLOOR_4    = 4
} Floor_t;

/* Elevator operating states — carried in IPC packet lift_state field */
typedef enum {
    LIFT_STATE_IDLE        = 0,
    LIFT_STATE_MOVING_UP   = 1,
    LIFT_STATE_MOVING_DOWN = 2,
    LIFT_STATE_DOORS_OPEN  = 3,
    LIFT_STATE_EMERGENCY   = 4
} LiftState_t;

/* Motor speed levels — values ARE the PWM duty-cycle percent */
typedef enum {
    MOTOR_REST       = 0,
    MOTOR_LOW_SPEED  = 20,
    MOTOR_HIGH_SPEED = 100
} Motor_Speed_t;

/* Emergency flag carried in IPC packet */
typedef enum {
    EMERGENCY_NORMAL = 0,
    EMERGENCY_STOP   = 1
} Emergency_t;

/* Cabin button bitmask encoding for IPC packet field */
/* Bit N-1 set means floor N is requested (bit 0 = floor 1 … bit 3 = floor 4) */
#define CABIN_BTN_NONE   (0x00U)
#define CABIN_BTN_FLOOR1 (1U << 0)
#define CABIN_BTN_FLOOR2 (1U << 1)
#define CABIN_BTN_FLOOR3 (1U << 2)
#define CABIN_BTN_FLOOR4 (1U << 3)

/* ================================================================
 * FSM timing constants
 * ================================================================ */
#define ELEVATOR_DOORS_OPEN_MS  3000U   /* Door stays open 3 s              */
#define ELEVATOR_SLOW_ZONE_MS    500U   /* Slow down 0.5 s before target    */
#define ELEVATOR_FSM_PERIOD_MS    10U   /* Scheduler calls FSM every 10 ms  */

/* ================================================================
 * FSM context — one instance per board
 * ================================================================ */
typedef struct {
    LiftState_t state;           /* Current FSM state                        */
    Floor_t     current_floor;   /* Last confirmed floor (from sensor)       */
    Floor_t     target_floor;    /* Active destination                       */
    Floor_t     queued_floors[5];/* Internal cabin request queue [1..4]      */
    uint8       queue_head;      /* Next floor to serve                      */
    uint8       queue_size;      /* Number of floors in queue                */
    uint32      door_timer_ms;   /* Counts up while doors are open           */
    boolean     emergency_active;/* Latched emergency flag                   */
} ElevatorFSM_t;

/* ================================================================
 * Public API
 * ================================================================ */

/* Call once during App_Init, after Motor_Init */
void ElevatorFSM_Init(ElevatorFSM_t *fsm);

/* Call every ELEVATOR_FSM_PERIOD_MS via the scheduler.
 * Reads global button/sensor/IPC flags; writes motor + IPC tx. */
void ElevatorFSM_Tick(ElevatorFSM_t *fsm);

/* Assign a target floor to this elevator (called by Dispatcher on Master,
 * or from IPC rx processing on Slave). Does nothing if emergency active. */
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor);

/* Read-only accessors used by Dispatcher and telemetry */
LiftState_t ElevatorFSM_GetState(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetCurrentFloor(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetTargetFloor(const ElevatorFSM_t *fsm);

#endif //STM32_TEMPLATE_ELEVATORFSM_H
