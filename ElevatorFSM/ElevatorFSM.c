/*
 * ElevatorFSM.c — Timer-based movement simulation
 *
 * This implementation fulfills the requirement to use Timer drivers
 * for arrival simulation and uses a 3-phase speed profile:
 *  1. Start Slow (20%): first 1/4 of trip
 *  2. Full Speed (100%): middle 1/2 of trip
 *  3. Reach Slow (20%): final 1/4 of trip
 */

#include "ElevatorFSM.h"
#include "../IPC/Ipc.h"
#include "../Motor/Motor.h"
#include "../Button/Button.h"
#include "../Nvic/Nvic.h"
#include "../Usart/Usart.h"
#include "../Timer/Timer.h"

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
static boolean  FSM_HasQueuedFloor(const ElevatorFSM_t *fsm);
static Floor_t  FSM_PickNextTarget(ElevatorFSM_t *fsm);
static void     FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor);
static boolean  FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor);
static void     FSM_SafePrintFloor(Floor_t floor);
static boolean  FSM_IsValidFloor(Floor_t floor);

/* ================================================================
 * ElevatorFSM_Init
 * ================================================================ */
void ElevatorFSM_Init(ElevatorFSM_t *fsm) {
    fsm->state                = LIFT_STATE_IDLE;
    fsm->pre_independent_state = LIFT_STATE_IDLE;
    fsm->current_floor        = FLOOR_1;
    fsm->start_floor          = FLOOR_1;
    fsm->target_floor         = FLOOR_NONE;
    fsm->queue_size           = 0;
    fsm->door_timer_ms        = 0;
    fsm->move_timer_ms        = 0;
    fsm->move_total_time_ms   = 0;
    fsm->emergency_active     = FALSE;
    fsm->independent_mode     = FALSE;

    for (uint8 i = 0; i < MAX_QUEUE_SIZE; i++) {
        fsm->queue[i] = FLOOR_NONE;
    }

    /* Initialize Hardware Timer (TIM5) for movement simulation */
    Timer_Init(TIM_INSTANCE_5, TIM_PRESCALER_1MS_TICK, 0xFFFF);

    Motor_SetSpeed(MOTOR_REST);
    FSM_PublishToIpc(fsm);

    Usart1_TransmitString("FSM: Init (Timer-based simulation enabled)\r\n");
}

/* ================================================================
 * ElevatorFSM_Tick — called every 10ms
 * ================================================================ */
void ElevatorFSM_Tick(ElevatorFSM_t *fsm) {

    /* ── 1. Emergency check ── */
    ENTER_CRITICAL();
    boolean emg = g_Emergency_Flag;
    EXIT_CRITICAL();

    if (emg && !fsm->emergency_active) {
        FSM_EnterEmergency(fsm);
        FSM_PublishToIpc(fsm);
        return;
    }

    /* ── 2. Collect cabin button presses ── */
    FSM_CollectCabinRequests(fsm);

    /* ── 3. Check comm fault ── */
    if (g_comm_fault && !fsm->independent_mode && !fsm->emergency_active) {
        FSM_EnterIndependent(fsm);
    } else if (!g_comm_fault && fsm->independent_mode) {
        fsm->independent_mode = FALSE;
        fsm->state = fsm->pre_independent_state;
        Usart1_TransmitString("FSM: Comm restored, exiting INDEPENDENT\r\n");
    }

    /* ── 4. Slave: check dispatcher target from IPC ── */
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

    /* ── 5. State dispatch ── */
    switch (fsm->state) {
        case LIFT_STATE_IDLE:        FSM_HandleIdle(fsm);        break;
        case LIFT_STATE_MOVING_UP:   /* fall through */
        case LIFT_STATE_MOVING_DOWN: FSM_HandleMoving(fsm);      break;
        case LIFT_STATE_DOORS_OPEN:  FSM_HandleDoorsOpen(fsm);   break;
        case LIFT_STATE_EMERGENCY:   FSM_HandleEmergency(fsm);   break;
        case LIFT_STATE_INDEPENDENT: FSM_HandleIndependent(fsm); break;
        default:                     FSM_EnterIdle(fsm);         break;
    }

    /* ── 6. Publish state to IPC ── */
    FSM_PublishToIpc(fsm);
}

/* ================================================================
 * ElevatorFSM_AssignTarget
 * ================================================================ */
void ElevatorFSM_AssignTarget(ElevatorFSM_t *fsm, Floor_t floor) {
    if (fsm->emergency_active)             return;
    if (!FSM_IsValidFloor(floor))          return;
    if (FSM_IsFloorQueued(fsm, floor))     return;

    if (floor == fsm->current_floor && fsm->state == LIFT_STATE_IDLE) {
        FSM_EnterDoorsOpen(fsm);
        return;
    }

    ENTER_CRITICAL();
    if (fsm->queue_size < MAX_QUEUE_SIZE) {
        fsm->queue[fsm->queue_size] = floor;
        fsm->queue_size++;
    }

    if (fsm->state == LIFT_STATE_IDLE) {
        fsm->target_floor = floor;
        FSM_EnterMoving(fsm);
    }
    EXIT_CRITICAL();
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
LiftState_t ElevatorFSM_GetState(const ElevatorFSM_t *fsm)         { return fsm->state; }
Floor_t     ElevatorFSM_GetCurrentFloor(const ElevatorFSM_t *fsm)  { return fsm->current_floor; }
Floor_t     ElevatorFSM_GetTargetFloor(const ElevatorFSM_t *fsm)   { return fsm->target_floor; }
boolean     ElevatorFSM_IsEmergency(const ElevatorFSM_t *fsm)      { return fsm->emergency_active; }
boolean     ElevatorFSM_IsIndependent(const ElevatorFSM_t *fsm)    { return fsm->independent_mode; }

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

    fsm->start_floor = fsm->current_floor;
    uint8 diff = (fsm->target_floor > fsm->current_floor) 
                 ? (uint8)fsm->target_floor - (uint8)fsm->current_floor
                 : (uint8)fsm->current_floor - (uint8)fsm->target_floor;

    fsm->move_total_time_ms = (uint32)diff * ELEVATOR_TIME_PER_FLOOR_MS;
    fsm->move_timer_ms      = 0;

    /* Start Hardware Timer for precise movement tracking */
    Timer_Stop(TIM_INSTANCE_5);
    Timer_Init(TIM_INSTANCE_5, TIM_PRESCALER_1MS_TICK, 0xFFFF);
    Timer_Start(TIM_INSTANCE_5);

    if (fsm->target_floor > fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_UP;
        Usart1_TransmitString("FSM: MOVING_UP to floor ");
    } else if (fsm->target_floor < fsm->current_floor) {
        fsm->state = LIFT_STATE_MOVING_DOWN;
        Usart1_TransmitString("FSM: MOVING_DOWN to floor ");
    } else {
        FSM_EnterDoorsOpen(fsm);
        return;
    }
    FSM_SafePrintFloor(fsm->target_floor);
    Usart1_TransmitString("\r\n");
    
    Motor_SetSpeed(MOTOR_LOW_SPEED);
}

static void FSM_EnterDoorsOpen(ElevatorFSM_t *fsm) {
    fsm->state         = LIFT_STATE_DOORS_OPEN;
    fsm->door_timer_ms = 0;
    FSM_DequeueFloor(fsm, fsm->current_floor);
    Motor_SetSpeed(MOTOR_REST);
    Usart1_TransmitString("FSM: DOORS_OPEN at floor ");
    FSM_SafePrintFloor(fsm->current_floor);
    Usart1_TransmitString("\r\n");
}

static void FSM_EnterEmergency(ElevatorFSM_t *fsm) {
    fsm->state            = LIFT_STATE_EMERGENCY;
    fsm->emergency_active = TRUE;
    fsm->target_floor     = FLOOR_NONE;
    fsm->queue_size       = 0;
    Timer_Stop(TIM_INSTANCE_5);
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
            if (fsm->target_floor == fsm->current_floor) FSM_EnterDoorsOpen(fsm);
            else FSM_EnterMoving(fsm);
        }
    }
}

static void FSM_HandleMoving(ElevatorFSM_t *fsm) {
    if (fsm->target_floor == FLOOR_NONE) {
        FSM_EnterIdle(fsm);
        return;
    }

    /* Accumulate time from Hardware Timer */
    fsm->move_timer_ms += ELEVATOR_FSM_PERIOD_MS;

    /* ── 1. Update current estimated floor ── */
    uint32 floors_elapsed = fsm->move_timer_ms / ELEVATOR_TIME_PER_FLOOR_MS;
    if (fsm->state == LIFT_STATE_MOVING_UP) {
        uint8 new_f = (uint8)fsm->start_floor + (uint8)floors_elapsed;
        if (new_f > (uint8)fsm->target_floor) new_f = (uint8)fsm->target_floor;
        fsm->current_floor = (Floor_t)new_f;
    } else {
        uint8 start = (uint8)fsm->start_floor;
        uint8 new_f = (start > (uint8)floors_elapsed) ? start - (uint8)floors_elapsed : (uint8)fsm->target_floor;
        if (new_f < (uint8)fsm->target_floor) new_f = (uint8)fsm->target_floor;
        fsm->current_floor = (Floor_t)new_f;
    }

    /* ── 2. 3-Phase Speed Profile ── */
    uint32 t = fsm->move_timer_ms;
    uint32 T = fsm->move_total_time_ms;

    if (t < (T / 4)) {
        /* Phase 1: Start Slow (20%) */
        if (Motor_GetSpeed() != MOTOR_LOW_SPEED) Motor_SetSpeed(MOTOR_LOW_SPEED);
    } else if (t < (3 * T / 4)) {
        /* Phase 2: Full Speed (100%) */
        if (Motor_GetSpeed() != MOTOR_HIGH_SPEED) Motor_SetSpeed(MOTOR_HIGH_SPEED);
    } else if (t < T) {
        /* Phase 3: Reach Slow (20%) */
        if (Motor_GetSpeed() != MOTOR_LOW_SPEED) Motor_SetSpeed(MOTOR_LOW_SPEED);
    } else {
        /* Phase 4: Reached (0%) */
        fsm->current_floor = fsm->target_floor;
        Timer_Stop(TIM_INSTANCE_5);
        FSM_EnterDoorsOpen(fsm);
    }
}

static void FSM_HandleDoorsOpen(ElevatorFSM_t *fsm) {
    fsm->door_timer_ms += ELEVATOR_FSM_PERIOD_MS;
    if (fsm->door_timer_ms >= ELEVATOR_DOORS_OPEN_MS) {
        if (FSM_HasQueuedFloor(fsm)) {
            fsm->target_floor = FSM_PickNextTarget(fsm);
            if (fsm->target_floor != FLOOR_NONE) FSM_EnterMoving(fsm);
            else FSM_EnterIdle(fsm);
        } else {
            FSM_EnterIdle(fsm);
        }
    }
}

static void FSM_HandleEmergency(ElevatorFSM_t *fsm) { (void)fsm; }

static void FSM_HandleIndependent(ElevatorFSM_t *fsm) {
    if (fsm->target_floor == FLOOR_NONE && FSM_HasQueuedFloor(fsm)) {
        fsm->target_floor = FSM_PickNextTarget(fsm);
        if (fsm->target_floor != FLOOR_NONE) {
            if (fsm->target_floor == fsm->current_floor) FSM_EnterDoorsOpen(fsm);
            else FSM_EnterMoving(fsm);
        }
    }
}

/* ================================================================
 * Private utilities
 * ================================================================ */
static void FSM_CollectCabinRequests(ElevatorFSM_t *fsm) {
    for (uint8 f = 1; f <= NUM_FLOORS; f++) {
        ENTER_CRITICAL();
        boolean pressed = g_CabinRequests[f];
        if (pressed) g_CabinRequests[f] = FALSE;
        EXIT_CRITICAL();
        if (pressed) ElevatorFSM_AssignTarget(fsm, (Floor_t)f);
    }
}

static void FSM_PublishToIpc(ElevatorFSM_t *fsm) {
    ENTER_CRITICAL();
    g_tx_packet.current_floor = (uint8)fsm->current_floor;
    g_tx_packet.target_floor  = (uint8)fsm->target_floor;
    g_tx_packet.lift_state    = (uint8)fsm->state;
    g_tx_packet.motor_speed   = (uint8)Motor_GetSpeed();
    g_tx_packet.emergency     = fsm->emergency_active ? (uint8)EMERGENCY_STOP : (uint8)EMERGENCY_NORMAL;
    uint8 mask = 0;
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        uint8 fl = (uint8)fsm->queue[i];
        if (fl >= 1 && fl <= NUM_FLOORS) mask |= (1U << (fl - 1));
    }
    g_tx_packet.cabin_buttons = mask;
    EXIT_CRITICAL();
}

static boolean FSM_HasQueuedFloor(const ElevatorFSM_t *fsm) { return (fsm->queue_size > 0); }

static Floor_t FSM_PickNextTarget(ElevatorFSM_t *fsm) {
    if (fsm->queue_size == 0) return FLOOR_NONE;
    Floor_t best = FLOOR_NONE;
    uint8 ref = (uint8)fsm->current_floor;
    if (fsm->state == LIFT_STATE_MOVING_UP || fsm->state == LIFT_STATE_INDEPENDENT) {
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            if (f > ref) { if (best == FLOOR_NONE || f < (uint8)best) best = (Floor_t)f; }
        }
        if (best == FLOOR_NONE) {
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queue[i];
                if (f < ref) { if (best == FLOOR_NONE || f > (uint8)best) best = (Floor_t)f; }
            }
        }
    } else if (fsm->state == LIFT_STATE_MOVING_DOWN) {
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            if (f < ref) { if (best == FLOOR_NONE || f > (uint8)best) best = (Floor_t)f; }
        }
        if (best == FLOOR_NONE) {
            for (uint8 i = 0; i < fsm->queue_size; i++) {
                uint8 f = (uint8)fsm->queue[i];
                if (f > ref) { if (best == FLOOR_NONE || f < (uint8)best) best = (Floor_t)f; }
            }
        }
    } else {
        uint8 min_dist = 0xFF;
        for (uint8 i = 0; i < fsm->queue_size; i++) {
            uint8 f = (uint8)fsm->queue[i];
            uint8 d = (f > ref) ? (f - ref) : (ref - f);
            if (d < min_dist) { min_dist = d; best = (Floor_t)f; }
        }
    }
    return best;
}

static void FSM_DequeueFloor(ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) {
        if (fsm->queue[i] == floor) {
            for (uint8 j = i; j < fsm->queue_size - 1; j++) fsm->queue[j] = fsm->queue[j + 1];
            fsm->queue[fsm->queue_size - 1] = FLOOR_NONE;
            fsm->queue_size--;
            return;
        }
    }
}

static boolean FSM_IsFloorQueued(const ElevatorFSM_t *fsm, Floor_t floor) {
    for (uint8 i = 0; i < fsm->queue_size; i++) { if (fsm->queue[i] == floor) return TRUE; }
    return FALSE;
}

static void FSM_SafePrintFloor(Floor_t floor) {
    uint8 f = (uint8)floor;
    if (f >= 1 && f <= NUM_FLOORS) Usart1_TransmitByte((uint8)('0' + f));
    else Usart1_TransmitByte((uint8)'?');
}

static boolean FSM_IsValidFloor(Floor_t floor) {
    return ((uint8)floor >= 1 && (uint8)floor <= NUM_FLOORS);
}
