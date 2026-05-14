//
// Created by Khalaf on 14/05/2026.
//

#include "ElevatorFSM.h"
#include "../Ipc/Ipc.h"
#include "../Motor/Motor.h"
#include "../Button/Button.h"
#include "../Nvic/Nvic.h"        /* ENTER_CRITICAL / EXIT_CRITICAL */

/* ================================================================
 * Private helpers — forward declarations
 * ================================================================ */
static void     FSM_EnterIdle(ElevatorFSM_t *fsm);
static void     FSM_EnterMoving(ElevatorFSM_t *fsm);
static void     FSM_EnterDoorsOpen(ElevatorFSM_t *fsm);
static void     FSM_EnterEmergency(ElevatorFSM_t *fsm);
static void     FSM_HandleIdle(ElevatorFSM_t *fsm);
static void     FSM_HandleMovingUp(ElevatorFSM_t *fsm);
static void     FSM_HandleMovingDown(ElevatorFSM_t *fsm);
static void     FSM_HandleDoorsOpen(ElevatorFSM_t *fsm);
static void     FSM_HandleEmergency(ElevatorFSM_t *fsm);
static void     FSM_CollectCabinRequests(ElevatorFSM_t *fsm);
static void     FSM_PublishToIpc(ElevatorFSM_t *fsm);
static boolean  FSM_HasQueuedFloor(const ElevatorFSM_t *fsm);
static Floor_t  FSM_NextFloorInDirection(ElevatorFSM_t *fsm);
static void     FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor);
static boolean  FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor);

/* ================================================================
 * ElevatorFSM_Init
 * ================================================================ */
void ElevatorFSM_Init(ElevatorFSM_t *fsm) {
    fsm->state            = LIFT_STATE_IDLE;
    fsm->current_floor    = FLOOR_1;        /* Assume start at ground floor  */
    fsm->target_floor     = FLOOR_NONE;
    fsm->queue_head       = 0;
    fsm->queue_size       = 0;
    fsm->door_timer_ms    = 0;
    fsm->emergency_active = FALSE;

    for (uint8 i = 0; i < 5; i++) {
        fsm->queued_floors[i] = FLOOR_NONE;
    }

    /* Sync floor sensor — if a sensor fired before FSM init, capture it */
    if (g_Current_Floor_Sensor >= 1 && g_Current_Floor_Sensor <= 4) {
        fsm->current_floor = (Floor_t)g_Current_Floor_Sensor;
    }

    Motor_SetSpeed(MOTOR_REST);
    FSM_PublishToIpc(fsm);

    Usart1_TransmitString("FSM: Initialized at floor ");
    char buf[4] = {'0' + (char)fsm->current_floor, '\r', '\n', '\0'};
    Usart1_TransmitString(buf);
}

/* ================================================================
 * ElevatorFSM_Tick  — called every ELEVATOR_FSM_PERIOD_MS (10 ms)
 * ================================================================ */
void ElevatorFSM_Tick(ElevatorFSM_t *fsm) {

    /* ── 1. Emergency check — highest priority, any state ── */
    if (g_Emergency_Flag && !fsm->emergency_active) {
        FSM_EnterEmergency(fsm);
        FSM_PublishToIpc(fsm);
        return;
    }

    /* ── 2. Update current floor from sensor (any state) ── */
    ENTER_CRITICAL();
    uint8 sensor_snapshot = g_Current_Floor_Sensor;
    EXIT_CRITICAL();

    if (sensor_snapshot >= 1 && sensor_snapshot <= 4) {
        fsm->current_floor = (Floor_t)sensor_snapshot;
    }

    /* ── 3. Collect new cabin button presses ── */
    FSM_CollectCabinRequests(fsm);

    /* ── 4. On Slave: check if Master assigned a new target via IPC ── */
#ifndef BUILD_AS_MASTER
    if (!fsm->emergency_active) {
        Floor_t ipc_target = (Floor_t)g_rx_packet.target_floor;
        if (ipc_target >= FLOOR_1 && ipc_target <= FLOOR_4) {
            if (!FSM_IsFloorQueued(fsm, ipc_target) &&
                ipc_target != fsm->current_floor) {
                ElevatorFSM_AssignTarget(fsm, ipc_target);
            }
        }
        /* Propagate emergency from Master */
        if (g_rx_packet.emergency == EMERGENCY_STOP) {
            FSM_EnterEmergency(fsm);
            FSM_PublishToIpc(fsm);
            return;
        }
    }
#endif

    /* ── 5. Dispatch to current state handler ── */
    switch (fsm->state) {
        case LIFT_STATE_IDLE:        FSM_HandleIdle(fsm);        break;
        case LIFT_STATE_MOVING_UP:   FSM_HandleMovingUp(fsm);    break;
        case LIFT_STATE_MOVING_DOWN: FSM_HandleMovingDown(fsm);  break;
        case LIFT_STATE_DOORS_OPEN:  FSM_HandleDoorsOpen(fsm);   break;
        case LIFT_STATE_EMERGENCY:   FSM_HandleEmergency(fsm);   break;
        default:                     FSM_EnterIdle(fsm);          break;
    }

    /* ── 6. Always publish updated state to IPC tx packet ── */
    FSM_PublishToIpc(fsm);
}

/* ================================================================
 * ElevatorFSM_AssignTarget
 * ================================================================ */
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor) {
    if (fsm->emergency_active)          return;
    if (floor < FLOOR_1 || floor > FLOOR_4) return;
    if (floor == fsm->current_floor &&
        fsm->state == LIFT_STATE_IDLE)  return; /* Already here and idle */
    if (FSM_IsFloorQueued(fsm, floor))  return; /* Already queued */

    ENTER_CRITICAL();
    /* Enqueue: store floors in order of insertion.
     * FSM_NextFloorInDirection picks the best one each time. */
    if (fsm->queue_size < 4) {
        fsm->queued_floors[fsm->queue_size] = floor;
        fsm->queue_size++;
    }

    /* If idle, immediately set target and start moving */
    if (fsm->state == LIFT_STATE_IDLE) {
        fsm->target_floor = floor;
        FSM_EnterMoving(fsm);
    }
    EXIT_CRITICAL();
}

/* ================================================================
 * Accessors
 * ================================================================ */
LiftState_t ElevatorFSM_GetState(const ElevatorFSM_t *fsm) {
    return fsm->state;
}
Floor_t ElevatorFSM_GetCurrentFloor(const ElevatorFSM_t *fsm) {
    return fsm->current_floor;
}
Floor_t ElevatorFSM_GetTargetFloor(const ElevatorFSM_t *fsm) {
    return fsm->target_floor;
}

/* ================================================================
 * State entry functions
 * ================================================================ */

static void FSM_EnterIdle(ElevatorFSM_t *fsm) {
    fsm->state        = LIFT_STATE_IDLE;
    fsm->target_floor = FLOOR_NONE;
    Motor_SetSpeed(MOTOR_REST);
    Usart1_TransmitString("FSM: IDLE\r\n");
}

static void FSM_EnterMoving(ElevatorFSM_t *fsm) {
    if (fsm->target_floor == FLOOR_NONE) return;

    if ((uint8)fsm->target_floor > (uint8)fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_UP;
        Motor_SetSpeed(MOTOR_HIGH_SPEED);
        Usart1_TransmitString("FSM: MOVING_UP\r\n");
    } else if ((uint8)fsm->target_floor < (uint8)fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_DOWN;
        Motor_SetSpeed(MOTOR_HIGH_SPEED);
        Usart1_TransmitString("FSM: MOVING_DOWN\r\n");
    } else {
        /* Already at target — open doors */
        FSM_EnterDoorsOpen(fsm);
    }
}

static void FSM_EnterDoorsOpen(ElevatorFSM_t *fsm) {
    fsm->state         = LIFT_STATE_DOORS_OPEN;
    fsm->door_timer_ms = 0;
    Motor_SetSpeed(MOTOR_REST);

    /* Remove the served floor from the queue */
    FSM_DequeueFloor(fsm, fsm->current_floor);

    Usart1_TransmitString("FSM: DOORS_OPEN\r\n");
}

static void FSM_EnterEmergency(ElevatorFSM_t *fsm) {
    fsm->state            = LIFT_STATE_EMERGENCY;
    fsm->emergency_active = TRUE;
    fsm->target_floor     = FLOOR_NONE;
    fsm->queue_size       = 0;
    fsm->queue_head       = 0;

    Motor_SetSpeed(MOTOR_REST);
    Usart1_TransmitString("FSM: EMERGENCY — ALL MOTION STOPPED\r\n");
}

/* ================================================================
 * State handler functions
 * ================================================================ */

static void FSM_HandleIdle(ElevatorFSM_t *fsm) {
    /* If we have a queued floor (e.g. cabin button pressed while idle),
     * pick the best next target and start moving. */
    if (FSM_HasQueuedFloor(fsm)) {
        fsm->target_floor = FSM_NextFloorInDirection(fsm);
        if (fsm->target_floor != FLOOR_NONE) {
            FSM_EnterMoving(fsm);
        }
    }
    /* On Master, comm fault: Dispatcher will push targets directly via
     * ElevatorFSM_AssignTarget, so no special handling needed here. */
}

static void FSM_HandleMovingUp(ElevatorFSM_t *fsm) {
    /* Slow down one floor before target */
    if (fsm->target_floor != FLOOR_NONE) {
        uint8 floors_away = (uint8)fsm->target_floor - (uint8)fsm->current_floor;
        if (floors_away <= 1) {
            Motor_SetSpeed(MOTOR_LOW_SPEED);
        }
    }

    /* Check if we have reached the target floor */
    if (fsm->current_floor == fsm->target_floor) {
        FSM_EnterDoorsOpen(fsm);
    }
}

static void FSM_HandleMovingDown(ElevatorFSM_t *fsm) {
    /* Slow down one floor before target */
    if (fsm->target_floor != FLOOR_NONE) {
        uint8 floors_away = (uint8)fsm->current_floor - (uint8)fsm->target_floor;
        if (floors_away <= 1) {
            Motor_SetSpeed(MOTOR_LOW_SPEED);
        }
    }

    /* Check if we have reached the target floor */
    if (fsm->current_floor == fsm->target_floor) {
        FSM_EnterDoorsOpen(fsm);
    }
}

static void FSM_HandleDoorsOpen(ElevatorFSM_t *fsm) {
    /* Accumulate time spent with doors open.
     * Called every ELEVATOR_FSM_PERIOD_MS. */
    fsm->door_timer_ms += ELEVATOR_FSM_PERIOD_MS;

    if (fsm->door_timer_ms >= ELEVATOR_DOORS_OPEN_MS) {
        /* Doors closing — pick next queued floor if any */
        if (FSM_HasQueuedFloor(fsm)) {
            fsm->target_floor = FSM_NextFloorInDirection(fsm);
            FSM_EnterMoving(fsm);
        } else {
            FSM_EnterIdle(fsm);
        }
    }
}

static void FSM_HandleEmergency(ElevatorFSM_t *fsm) {
    /* Motor already stopped in EnterEmergency.
     * Latched until board reset — nothing to do per tick. */
    (void)fsm;
}

/* ================================================================
 * Private utility functions
 * ================================================================ */

/* Scan g_CabinRequests and enqueue any newly pressed floors */
static void FSM_CollectCabinRequests(ElevatorFSM_t *fsm) {
    for (uint8 floor = 1; floor <= 4; floor++) {
        ENTER_CRITICAL();
        boolean pressed = g_CabinRequests[floor];
        if (pressed) {
            g_CabinRequests[floor] = FALSE; /* Consume the request */
        }
        EXIT_CRITICAL();

        if (pressed) {
            ElevatorFSM_AssignTarget(fsm, (Floor_t)floor);
        }
    }
}

/* Build and write all fields of g_tx_packet from current FSM state.
 * Called at the end of every tick. */
static void FSM_PublishToIpc(ElevatorFSM_t *fsm) {
    ENTER_CRITICAL();

    g_tx_packet.current_floor = (uint8)fsm->current_floor;
    g_tx_packet.target_floor  = (uint8)fsm->target_floor;
    g_tx_packet.lift_state    = (uint8)fsm->state;
    g_tx_packet.motor_speed   = (uint8)Motor_GetSpeed();
    g_tx_packet.emergency     = fsm->emergency_active
                                    ? (uint8)EMERGENCY_STOP
                                    : (uint8)EMERGENCY_NORMAL;

    /* Pack cabin request queue into bitmask */
    uint8 btn_mask = CABIN_BTN_NONE;
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        uint8 f = (uint8)fsm->queued_floors[i];
        if (f >= 1 && f <= 4) {
            btn_mask |= (1U << (f - 1));
        }
    }
    g_tx_packet.cabin_buttons = btn_mask;

    EXIT_CRITICAL();
}

/* Returns TRUE if there is at least one floor in the queue */
static boolean FSM_HasQueuedFloor(const ElevatorFSM_t *fsm) {
    return (fsm->queue_size > 0);
}

/* Pick the next floor to serve using directional scan (SCAN/elevator algorithm):
 *  - If moving up  : lowest queued floor ABOVE current (or above target) first.
 *  - If moving down: highest queued floor BELOW current first.
 *  - If idle       : closest queued floor. */
static Floor_t FSM_NextFloorInDirection(ElevatorFSM_t *fsm) {
    if (fsm->queue_size == 0) return FLOOR_NONE;

    Floor_t best    = FLOOR_NONE;
    uint8   ref     = (uint8)fsm->current_floor;

    if (fsm->state == LIFT_STATE_MOVING_UP ||
        (fsm->state == LIFT_STATE_IDLE &&
         fsm->target_floor != FLOOR_NONE &&
         (uint8)fsm->target_floor > ref)) {
        /* Prefer lowest floor above current */
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queued_floors[i];
            if (f > ref) {
                if (best == FLOOR_NONE || f < (uint8)best) {
                    best = (Floor_t)f;
                }
            }
        }
        if (best == FLOOR_NONE) {
            /* No floors above — reverse: pick highest floor below */
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queued_floors[i];
                if (f < ref) {
                    if (best == FLOOR_NONE || f > (uint8)best) {
                        best = (Floor_t)f;
                    }
                }
            }
        }
    } else {
        /* Moving down or idle: prefer highest floor below current */
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queued_floors[i];
            if (f < ref) {
                if (best == FLOOR_NONE || f > (uint8)best) {
                    best = (Floor_t)f;
                }
            }
        }
        if (best == FLOOR_NONE) {
            /* No floors below — reverse: pick lowest floor above */
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queued_floors[i];
                if (f > ref) {
                    if (best == FLOOR_NONE || f < (uint8)best) {
                        best = (Floor_t)f;
                    }
                }
            }
        }
    }

    /* Idle with no direction preference: pick closest */
    if (fsm->state == LIFT_STATE_IDLE && best == FLOOR_NONE) {
        uint8 min_dist = 0xFF;
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f    = (uint8)fsm->queued_floors[i];
            uint8 dist = (f > ref) ? (f - ref) : (ref - f);
            if (dist < min_dist) {
                min_dist = dist;
                best     = (Floor_t)f;
            }
        }
    }

    return best;
}

/* Remove a specific floor from the queue */
static void FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        if (fsm->queued_floors[i] == floor) {
            /* Shift remaining entries left */
            for (uint8 j = i; j < fsm->queue_size - 1; j++) {
                fsm->queued_floors[j] = fsm->queued_floors[j + 1];
            }
            fsm->queued_floors[fsm->queue_size - 1] = FLOOR_NONE;
            fsm->queue_size--;
            return;
        }
    }
}

/* Check whether a floor is already in the queue */
static boolean FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        if (fsm->queued_floors[i] == floor) return TRUE;
    }
    return FALSE;
}