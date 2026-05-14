/*
 * ElevatorFSM.h
 * Finite State Machine for a single elevator cabin.
 */

#ifndef ELEVATOR_FSM_H
#define ELEVATOR_FSM_H

#include "../include/STD_TYPES.h"

/* ================================================================
 * Constants
 * ================================================================ */
#define NUM_FLOORS                 4U
#define ELEVATOR_TIME_PER_FLOOR_MS 50U   /* Parameter: 2s per floor          */
#define ELEVATOR_DOORS_OPEN_MS     50U   /* Door dwell time: 3 seconds       */
#define ELEVATOR_FSM_PERIOD_MS     10U     /* Scheduler calls FSM every 10 ms  */
#define MAX_QUEUE_SIZE             NUM_FLOORS

/* ================================================================
 * Floor identifiers
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
 * Motor speed levels
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
 * FSM context
 * ================================================================ */
typedef struct {
    LiftState_t     state;
    LiftState_t     pre_independent_state;
    Floor_t         current_floor;
    Floor_t         start_floor;
    Floor_t         target_floor;
    Floor_t         queue[MAX_QUEUE_SIZE];
    uint8           queue_size;
    uint32          door_timer_ms;
    uint32          move_timer_ms;
    uint32          move_total_time_ms;
    boolean         emergency_active;
    boolean         independent_mode;
} ElevatorFSM_t;

/* ================================================================
 * Public API
 * ================================================================ */
void ElevatorFSM_Init(ElevatorFSM_t *fsm);
void ElevatorFSM_Tick(ElevatorFSM_t *fsm);
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor);
void ElevatorFSM_ResetEmergency(ElevatorFSM_t *fsm);

/* Accessors */
LiftState_t ElevatorFSM_GetState(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetCurrentFloor(const ElevatorFSM_t *fsm);
Floor_t     ElevatorFSM_GetTargetFloor(const ElevatorFSM_t *fsm);
boolean     ElevatorFSM_IsEmergency(const ElevatorFSM_t *fsm);
boolean     ElevatorFSM_IsIndependent(const ElevatorFSM_t *fsm);

#endif /* ELEVATOR_FSM_H */
