/*
 * ElevatorFSM.c — Full FSM implementation (rewritten from scratch)
 *
 * Corner cases handled:
 *  - Emergency pre-empts ANY state; latched until explicit reset
 *  - INDEPENDENT_MODE on SPI comm fault (cabin-only queue)
 *  - Floor sensor values clamped to [1..4]; UART prints guarded
 *  - Duplicate cabin requests deduplicated
 *  - PWM ramp via RampState_t state machine (non-blocking)
 *  - Queue overflow silently rejected (max 4 entries)
 *  - Slave receives dispatcher target from IPC rx packet
 *
 * Volatile flags used:
 *  g_CabinRequests[], g_Current_Floor_Sensor, g_Emergency_Flag,
 *  g_tx_packet, g_rx_packet, g_comm_fault
 *
 * Driver API assumptions:
 *  Motor_SetSpeed(), Motor_GetSpeed(), ENTER/EXIT_CRITICAL(),
 *  Usart1_TransmitString(), Usart1_TransmitByte()
 */

#include "ElevatorFSM.h"
#include "../IPC/Ipc.h"
#include "../Motor/Motor.h"
#include "../Button/Button.h"
#include "../Nvic/Nvic.h"
#include "../Usart/Usart.h"

/* ================================================================
 * Forward declarations
 * ================================================================ */
static void     FSM_EnterIdle(ElevatorFSM_t *fsm);
static void     FSM_EnterMoving(ElevatorFSM_t *fsm);
static void     FSM_EnterDoorsOpen(ElevatorFSM_t *fsm);
static void     FSM_EnterEmergency(ElevatorFSM_t *fsm);
static void     FSM_EnterIndependent(ElevatorFSM_t *fsm);
static void     FSM_HandleIdle(ElevatorFSM_t *fsm);
static void     FSM_HandleMoving(ElevatorFSM_t *fsm);
static void     FSM_HandleDoorsOpen(ElevatorFSM_t *fsm);
static void     FSM_HandleEmergency(ElevatorFSM_t *fsm);
static void     FSM_HandleIndependent(ElevatorFSM_t *fsm);
static void     FSM_CollectCabinRequests(ElevatorFSM_t *fsm);
static void     FSM_PublishToIpc(ElevatorFSM_t *fsm);
static void     FSM_UpdateFloorFromSensor(ElevatorFSM_t *fsm);
static boolean  FSM_HasQueuedFloor(const ElevatorFSM_t *fsm);
static Floor_t  FSM_PickNextTarget(ElevatorFSM_t *fsm);
static void     FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor);
static boolean  FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor);
static void     FSM_StartRamp(ElevatorFSM_t *fsm, RampDirection_t dir);
static void     FSM_SafePrintFloor(Floor_t floor);
static boolean  FSM_IsValidFloor(Floor_t floor);

/* ================================================================
 * ElevatorFSM_Init
 * ================================================================ */
void ElevatorFSM_Init(ElevatorFSM_t *fsm) {
    fsm->state                = LIFT_STATE_IDLE;
    fsm->pre_independent_state = LIFT_STATE_IDLE;
    fsm->current_floor        = FLOOR_1;
    fsm->target_floor         = FLOOR_NONE;
    fsm->queue_size           = 0;
    fsm->door_timer_ms        = 0;
    fsm->emergency_active     = FALSE;
    fsm->independent_mode     = FALSE;
    fsm->ramp.direction       = RAMP_NONE;
    fsm->ramp.step_count      = 0;
    fsm->ramp.target_duty     = 0;
    fsm->ramp.current_duty    = 0;

    for (uint8 i = 0; i < MAX_QUEUE_SIZE; i++) {
        fsm->queue[i] = FLOOR_NONE;
    }

    /* Sync from sensor if it fired before init */
    ENTER_CRITICAL();
    uint8 sensor = g_Current_Floor_Sensor;
    EXIT_CRITICAL();
    if (sensor >= 1 && sensor <= NUM_FLOORS) {
        fsm->current_floor = (Floor_t)sensor;
    }

    Motor_SetSpeed(MOTOR_REST);
    FSM_PublishToIpc(fsm);

    Usart1_TransmitString("FSM: Init at floor ");
    FSM_SafePrintFloor(fsm->current_floor);
    Usart1_TransmitString("\r\n");
}

/* ================================================================
 * ElevatorFSM_Tick — called every 10ms
 * ================================================================ */
void ElevatorFSM_Tick(ElevatorFSM_t *fsm) {

    /* ── 1. Emergency check — highest priority, any state ── */
    ENTER_CRITICAL();
    boolean emg = g_Emergency_Flag;
    EXIT_CRITICAL();

    if (emg && !fsm->emergency_active) {
        FSM_EnterEmergency(fsm);
        FSM_PublishToIpc(fsm);
        return;
    }

    /* ── 2. Update current floor from sensor ── */
    FSM_UpdateFloorFromSensor(fsm);

    /* ── 3. Collect cabin button presses ── */
    FSM_CollectCabinRequests(fsm);

    /* ── 4. Check comm fault for INDEPENDENT_MODE ── */
    if (g_comm_fault && !fsm->independent_mode && !fsm->emergency_active) {
        FSM_EnterIndependent(fsm);
    } else if (!g_comm_fault && fsm->independent_mode) {
        /* Comm restored — exit independent mode */
        fsm->independent_mode = FALSE;
        fsm->state = fsm->pre_independent_state;
        Usart1_TransmitString("FSM: Comm restored, exiting INDEPENDENT\r\n");
    }

    /* ── 5. Slave: check dispatcher target from IPC ── */
#ifndef BUILD_AS_MASTER
    if (!fsm->emergency_active && !fsm->independent_mode) {
        ENTER_CRITICAL();
        uint8 ipc_tgt = g_rx_packet.target_floor;
        uint8 ipc_emg = g_rx_packet.emergency;
        EXIT_CRITICAL();

        if (ipc_emg == (uint8)EMERGENCY_STOP) {
            FSM_EnterEmergency(fsm);
            FSM_PublishToIpc(fsm);
            return;
        }

        Floor_t ipc_target = (Floor_t)ipc_tgt;
        if (FSM_IsValidFloor(ipc_target)) {
            if (!FSM_IsFloorQueued(fsm, ipc_target) &&
                ipc_target != fsm->current_floor) {
                ElevatorFSM_AssignTarget(fsm, ipc_target);
            }
        }
    }
#endif

    /* ── 6. State dispatch ── */
    switch (fsm->state) {
        case LIFT_STATE_IDLE:        FSM_HandleIdle(fsm);        break;
        case LIFT_STATE_MOVING_UP:   /* fall through */
        case LIFT_STATE_MOVING_DOWN: FSM_HandleMoving(fsm);      break;
        case LIFT_STATE_DOORS_OPEN:  FSM_HandleDoorsOpen(fsm);   break;
        case LIFT_STATE_EMERGENCY:   FSM_HandleEmergency(fsm);   break;
        case LIFT_STATE_INDEPENDENT: FSM_HandleIndependent(fsm); break;
        default:                     FSM_EnterIdle(fsm);         break;
    }

    /* ── 7. Publish state to IPC ── */
    FSM_PublishToIpc(fsm);
}

/* ================================================================
 * ElevatorFSM_AssignTarget
 * ================================================================ */
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor) {
    if (fsm->emergency_active)             return;
    if (!FSM_IsValidFloor(floor))          return;
    if (FSM_IsFloorQueued(fsm, floor))     return;

    /* Already at this floor and idle — open doors instead */
    if (floor == fsm->current_floor && fsm->state == LIFT_STATE_IDLE) {
        FSM_EnterDoorsOpen(fsm);
        return;
    }

    ENTER_CRITICAL();
    if (fsm->queue_size < MAX_QUEUE_SIZE) {
        fsm->queue[fsm->queue_size] = floor;
        fsm->queue_size++;
    }

    /* If idle, start moving immediately */
    if (fsm->state == LIFT_STATE_IDLE) {
        fsm->target_floor = floor;
        FSM_EnterMoving(fsm);
    }
    EXIT_CRITICAL();
}

/* ================================================================
 * ElevatorFSM_RampTick — called every 50ms by scheduler
 * ================================================================ */
void ElevatorFSM_RampTick(ElevatorFSM_t *fsm) {
    if (fsm->ramp.direction == RAMP_NONE) return;

    fsm->ramp.step_count++;

    if (fsm->ramp.direction == RAMP_UP) {
        /* 0% → 20% at step 5, → 100% at step 10 */
        if (fsm->ramp.step_count <= (ELEVATOR_RAMP_STEPS / 2)) {
            fsm->ramp.current_duty = MOTOR_LOW_SPEED;
        } else {
            fsm->ramp.current_duty = MOTOR_HIGH_SPEED;
        }
    } else { /* RAMP_DOWN */
        /* 100% → 20% at step 5, → 0% at step 10 */
        if (fsm->ramp.step_count <= (ELEVATOR_RAMP_STEPS / 2)) {
            fsm->ramp.current_duty = MOTOR_LOW_SPEED;
        } else {
            fsm->ramp.current_duty = MOTOR_REST;
        }
    }

    Motor_SetSpeed((Motor_Speed_t)fsm->ramp.current_duty);

    if (fsm->ramp.step_count >= ELEVATOR_RAMP_STEPS) {
        Motor_SetSpeed((Motor_Speed_t)fsm->ramp.target_duty);
        fsm->ramp.current_duty = fsm->ramp.target_duty;
        fsm->ramp.direction    = RAMP_NONE;
        fsm->ramp.step_count   = 0;
    }
}

/* ================================================================
 * ElevatorFSM_ResetEmergency
 * ================================================================ */
void ElevatorFSM_ResetEmergency(ElevatorFSM_t *fsm) {
    if (!fsm->emergency_active) return;

    ENTER_CRITICAL();
    g_Emergency_Flag = FALSE;
    EXIT_CRITICAL();

    fsm->emergency_active = FALSE;
    FSM_EnterIdle(fsm);
    Usart1_TransmitString("FSM: Emergency RESET\r\n");
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
boolean ElevatorFSM_IsEmergency(const ElevatorFSM_t *fsm) {
    return fsm->emergency_active;
}
boolean ElevatorFSM_IsIndependent(const ElevatorFSM_t *fsm) {
    return fsm->independent_mode;
}

/* ================================================================
 * State entry functions
 * ================================================================ */
static void FSM_EnterIdle(ElevatorFSM_t *fsm) {
    fsm->state        = LIFT_STATE_IDLE;
    fsm->target_floor = FLOOR_NONE;
    Motor_SetSpeed(MOTOR_REST);
    fsm->ramp.direction = RAMP_NONE;
    Usart1_TransmitString("FSM: IDLE\r\n");
}

static void FSM_EnterMoving(ElevatorFSM_t *fsm) {
    if (fsm->target_floor == FLOOR_NONE) return;

    if ((uint8)fsm->target_floor > (uint8)fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_UP;
        Usart1_TransmitString("FSM: MOVING_UP to floor ");
    } else if ((uint8)fsm->target_floor < (uint8)fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_DOWN;
        Usart1_TransmitString("FSM: MOVING_DOWN to floor ");
    } else {
        FSM_EnterDoorsOpen(fsm);
        return;
    }
    FSM_SafePrintFloor(fsm->target_floor);
    Usart1_TransmitString("\r\n");

    /* Start PWM ramp up: 0% → 20% → 100% */
    FSM_StartRamp(fsm, RAMP_UP);
}

static void FSM_EnterDoorsOpen(ElevatorFSM_t *fsm) {
    fsm->state         = LIFT_STATE_DOORS_OPEN;
    fsm->door_timer_ms = 0;

    /* Dequeue served floor */
    FSM_DequeueFloor(fsm, fsm->current_floor);

    /* Start PWM ramp down if motor is running */
    if (Motor_GetSpeed() != MOTOR_REST) {
        FSM_StartRamp(fsm, RAMP_DOWN);
    } else {
        Motor_SetSpeed(MOTOR_REST);
    }

    Usart1_TransmitString("FSM: DOORS_OPEN at floor ");
    FSM_SafePrintFloor(fsm->current_floor);
    Usart1_TransmitString("\r\n");
}

static void FSM_EnterEmergency(ElevatorFSM_t *fsm) {
    fsm->state            = LIFT_STATE_EMERGENCY;
    fsm->emergency_active = TRUE;
    fsm->target_floor     = FLOOR_NONE;
    fsm->queue_size       = 0;
    fsm->ramp.direction   = RAMP_NONE;

    Motor_SetSpeed(MOTOR_REST);
    Usart1_TransmitString("FSM: *** EMERGENCY STOP ***\r\n");
}

static void FSM_EnterIndependent(ElevatorFSM_t *fsm) {
    fsm->pre_independent_state = fsm->state;
    fsm->independent_mode      = TRUE;
    fsm->state                 = LIFT_STATE_INDEPENDENT;
    Usart1_TransmitString("FSM: INDEPENDENT_MODE (comm fault)\r\n");
}

/* ================================================================
 * State handlers
 * ================================================================ */
static void FSM_HandleIdle(ElevatorFSM_t *fsm) {
    if (FSM_HasQueuedFloor(fsm)) {
        fsm->target_floor = FSM_PickNextTarget(fsm);
        if (fsm->target_floor != FLOOR_NONE) {
            if (fsm->target_floor == fsm->current_floor) {
                FSM_EnterDoorsOpen(fsm);
            } else {
                FSM_EnterMoving(fsm);
            }
        }
    }
}

static void FSM_HandleMoving(ElevatorFSM_t *fsm) {
    if (fsm->target_floor == FLOOR_NONE) {
        FSM_EnterIdle(fsm);
        return;
    }

    /* Slow down when 1 floor away (approaching) */
    uint8 cur = (uint8)fsm->current_floor;
    uint8 tgt = (uint8)fsm->target_floor;
    uint8 dist = (cur > tgt) ? (cur - tgt) : (tgt - cur);

    if (dist <= 1 && fsm->ramp.direction == RAMP_NONE) {
        Motor_SetSpeed(MOTOR_LOW_SPEED);
    }

    /* Arrived at target floor */
    if (fsm->current_floor == fsm->target_floor) {
        FSM_EnterDoorsOpen(fsm);
    }
}

static void FSM_HandleDoorsOpen(ElevatorFSM_t *fsm) {
    fsm->door_timer_ms += ELEVATOR_FSM_PERIOD_MS;

    if (fsm->door_timer_ms >= ELEVATOR_DOORS_OPEN_MS) {
        if (FSM_HasQueuedFloor(fsm)) {
            fsm->target_floor = FSM_PickNextTarget(fsm);
            if (fsm->target_floor != FLOOR_NONE) {
                FSM_EnterMoving(fsm);
            } else {
                FSM_EnterIdle(fsm);
            }
        } else {
            FSM_EnterIdle(fsm);
        }
    }
}

static void FSM_HandleEmergency(ElevatorFSM_t *fsm) {
    /* Latched — do nothing until explicit reset */
    (void)fsm;
}

static void FSM_HandleIndependent(ElevatorFSM_t *fsm) {
    /* In independent mode, serve cabin-only queue like IDLE/MOVING */
    if (fsm->target_floor == FLOOR_NONE && FSM_HasQueuedFloor(fsm)) {
        fsm->target_floor = FSM_PickNextTarget(fsm);
        if (fsm->target_floor != FLOOR_NONE) {
            if (fsm->target_floor == fsm->current_floor) {
                FSM_EnterDoorsOpen(fsm);
                fsm->state = LIFT_STATE_INDEPENDENT; /* Stay independent */
            } else {
                FSM_EnterMoving(fsm);
                fsm->state = LIFT_STATE_INDEPENDENT; /* Stay independent */
            }
        }
    }

    /* Check arrival */
    if (fsm->target_floor != FLOOR_NONE &&
        fsm->current_floor == fsm->target_floor) {
        Motor_SetSpeed(MOTOR_REST);
        FSM_DequeueFloor(fsm, fsm->current_floor);
        fsm->target_floor = FLOOR_NONE;
        Usart1_TransmitString("FSM(IND): Arrived at floor ");
        FSM_SafePrintFloor(fsm->current_floor);
        Usart1_TransmitString("\r\n");
    }
}

/* ================================================================
 * Private utilities
 * ================================================================ */

static void FSM_UpdateFloorFromSensor(ElevatorFSM_t *fsm) {
    ENTER_CRITICAL();
    uint8 sensor = g_Current_Floor_Sensor;
    EXIT_CRITICAL();

    if (sensor >= 1 && sensor <= NUM_FLOORS) {
        fsm->current_floor = (Floor_t)sensor;
    }
}

static void FSM_CollectCabinRequests(ElevatorFSM_t *fsm) {
    for (uint8 f = 1; f <= NUM_FLOORS; f++) {
        ENTER_CRITICAL();
        boolean pressed = g_CabinRequests[f];
        if (pressed) g_CabinRequests[f] = FALSE;
        EXIT_CRITICAL();

        if (pressed) {
            ElevatorFSM_AssignTarget(fsm, (Floor_t)f);
        }
    }
}

static void FSM_PublishToIpc(ElevatorFSM_t *fsm) {
    ENTER_CRITICAL();

    g_tx_packet.current_floor = (uint8)fsm->current_floor;
    g_tx_packet.target_floor  = (uint8)fsm->target_floor;
    g_tx_packet.lift_state    = (uint8)fsm->state;
    g_tx_packet.motor_speed   = (uint8)Motor_GetSpeed();
    g_tx_packet.emergency     = fsm->emergency_active
                                    ? (uint8)EMERGENCY_STOP
                                    : (uint8)EMERGENCY_NORMAL;

    /* Pack cabin queue into bitmask */
    uint8 mask = CABIN_BTN_NONE;
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        uint8 fl = (uint8)fsm->queue[i];
        if (fl >= 1 && fl <= NUM_FLOORS) {
            mask |= (1U << (fl - 1));
        }
    }
    g_tx_packet.cabin_buttons = mask;

    EXIT_CRITICAL();
}

static boolean FSM_HasQueuedFloor(const ElevatorFSM_t *fsm) {
    return (fsm->queue_size > 0) ? TRUE : FALSE;
}

/* SCAN algorithm: prefer floors in current direction, reverse at end */
static Floor_t FSM_PickNextTarget(ElevatorFSM_t *fsm) {
    if (fsm->queue_size == 0) return FLOOR_NONE;

    Floor_t best = FLOOR_NONE;
    uint8 ref = (uint8)fsm->current_floor;

    if (fsm->state == LIFT_STATE_MOVING_UP || fsm->state == LIFT_STATE_INDEPENDENT) {
        /* Prefer lowest floor above current */
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            if (f > ref) {
                if (best == FLOOR_NONE || f < (uint8)best) best = (Floor_t)f;
            }
        }
        if (best == FLOOR_NONE) {
            /* Reverse: highest floor below */
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queue[i];
                if (f < ref) {
                    if (best == FLOOR_NONE || f > (uint8)best) best = (Floor_t)f;
                }
            }
        }
    } else if (fsm->state == LIFT_STATE_MOVING_DOWN) {
        /* Prefer highest floor below current */
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            if (f < ref) {
                if (best == FLOOR_NONE || f > (uint8)best) best = (Floor_t)f;
            }
        }
        if (best == FLOOR_NONE) {
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queue[i];
                if (f > ref) {
                    if (best == FLOOR_NONE || f < (uint8)best) best = (Floor_t)f;
                }
            }
        }
    } else {
        /* IDLE: pick closest */
        uint8 min_dist = 0xFF;
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            uint8 d = (f > ref) ? (f - ref) : (ref - f);
            if (d < min_dist) {
                min_dist = d;
                best = (Floor_t)f;
            }
        }
    }

    return best;
}

static void FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        if (fsm->queue[i] == floor) {
            for (uint8 j = i; j < fsm->queue_size - 1; j++) {
                fsm->queue[j] = fsm->queue[j + 1];
            }
            fsm->queue[fsm->queue_size - 1] = FLOOR_NONE;
            fsm->queue_size--;
            return;
        }
    }
}

static boolean FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        if (fsm->queue[i] == floor) return TRUE;
    }
    return FALSE;
}

static void FSM_StartRamp(ElevatorFSM_t *fsm, RampDirection_t dir) {
    fsm->ramp.direction  = dir;
    fsm->ramp.step_count = 0;
    if (dir == RAMP_UP) {
        fsm->ramp.current_duty = MOTOR_REST;
        fsm->ramp.target_duty  = MOTOR_HIGH_SPEED;
        Motor_SetSpeed(MOTOR_LOW_SPEED); /* Immediate first step */
    } else {
        fsm->ramp.current_duty = MOTOR_HIGH_SPEED;
        fsm->ramp.target_duty  = MOTOR_REST;
        Motor_SetSpeed(MOTOR_LOW_SPEED);
    }
}

/** Guard: only print floor if in valid range [1..4]; else print '?' */
static void FSM_SafePrintFloor(Floor_t floor) {
    uint8 f = (uint8)floor;
    if (f >= 1 && f <= NUM_FLOORS) {
        char c = (char)('0' + f);
        Usart1_TransmitByte((uint8)c);
    } else {
        Usart1_TransmitByte((uint8)'?');
        Usart1_TransmitString("[ERR:floor=");
        char c = (char)('0' + (f % 10));
        Usart1_TransmitByte((uint8)c);
        Usart1_TransmitString("]");
    }
}

static boolean FSM_IsValidFloor(Floor_t floor) {
    return ((uint8)floor >= 1 && (uint8)floor <= NUM_FLOORS) ? TRUE : FALSE;
}
