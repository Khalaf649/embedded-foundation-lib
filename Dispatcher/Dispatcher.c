/*
 * Dispatcher.c — Master-only hallway call dispatcher (rewritten from scratch)
 *
 * Priority algorithm (in order):
 *  1. COMM FAULT   — all calls to Elevator A; Slave = INDEPENDENT
 *  2. IMMEDIATE    — elevator IDLE and AT call_floor
 *  3. PERFECT MATCH— moving TOWARD call_floor in SAME direction
 *  4. PASSED MATCH — same direction, already passed call_floor
 *  5. OPPOSITE DIR — moving AWAY; excluded until path complete
 *  6. IDLE NEAREST — closest idle elevator; tie-break: prefer A
 *
 * Corner cases:
 *  - Both EMERGENCY: queue call, serve on recovery
 *  - Duplicate call: deduplicated via pending table
 *  - EMERGENCY mid-travel: re-dispatch that call
 *  - Slave INDEPENDENT → reassign unserved calls to A
 *  - Invalid call_floor: log error, discard
 *  - Stale SPI snapshot (>150ms): treat as comm fault
 */

#ifdef BUILD_AS_MASTER

#include "Dispatcher.h"
#include "../IPC/Ipc.h"
#include "../Button/Button.h"
#include "../Nvic/Nvic.h"
#include "../Usart/Usart.h"
#include "../Scheduler/Scheduler.h"

/* ================================================================
 * Private state
 * ================================================================ */
static ElevatorFSM_t  *s_fsm_a = (void*)0;       /* Local elevator (A) */
static PendingCall_t    s_calls[MAX_PENDING_CALLS]; /* Pending call table */
static uint8            s_call_count = 0;
static Floor_t          s_last_slave_target = FLOOR_NONE;
static uint32           s_last_good_spi_tick = 0;   /* Tick of last valid rx */

/* ================================================================
 * Forward declarations
 * ================================================================ */
static uint8   Score_Elevator(LiftState_t state, Floor_t current,
                              Floor_t target, Floor_t call_floor,
                              CallDirection_t call_dir);
static uint8   Floor_Distance(Floor_t a, Floor_t b);
static void    Process_Call(Floor_t call_floor, CallDirection_t dir);
static boolean Is_Call_Pending(Floor_t floor, CallDirection_t dir);
static void    Add_Pending_Call(Floor_t floor, CallDirection_t dir);
static void    SafePrintFloor(Floor_t floor);

/* ================================================================
 * Dispatcher_Init
 * ================================================================ */
void Dispatcher_Init(ElevatorFSM_t *fsm_a) {
    s_fsm_a             = fsm_a;
    s_call_count        = 0;
    s_last_slave_target = FLOOR_NONE;
    s_last_good_spi_tick = Scheduler_GetTick();

    for (uint8 i = 0; i < MAX_PENDING_CALLS; i++) {
        s_calls[i].active = FALSE;
    }
}

/* ================================================================
 * Dispatcher_Tick — called every 10ms
 * ================================================================ */
void Dispatcher_Tick(void) {
    /* ── Update SPI age tracking ── */
    if (!g_comm_fault) {
        s_last_good_spi_tick = Scheduler_GetTick();
    }

    uint32 now = Scheduler_GetTick();
    boolean spi_stale = ((now - s_last_good_spi_tick) > DISPATCHER_SPI_MAX_AGE_MS)
                        ? TRUE : FALSE;
    boolean use_comm_fault = (g_comm_fault || spi_stale) ? TRUE : FALSE;

    /* ── Check for elevator-in-EMERGENCY reassignment ── */
    if (ElevatorFSM_GetState(s_fsm_a) == LIFT_STATE_EMERGENCY) {
        Dispatcher_ReassignCalls(0); /* Reassign calls from A */
    }

    /* Check slave emergency from IPC */
    ENTER_CRITICAL();
    uint8 slave_emg = g_rx_packet.emergency;
    EXIT_CRITICAL();

    if (slave_emg == (uint8)EMERGENCY_STOP || use_comm_fault) {
        Dispatcher_ReassignCalls(1); /* Reassign calls from B */
    }

    /* ── Consume hallway button flags ── */
    /* UP calls: floors 1, 2, 3 */
    for (uint8 floor = 1; floor <= 3; floor++) {
        ENTER_CRITICAL();
        boolean up_req = g_HallwayUpRequests[floor];
        if (up_req) g_HallwayUpRequests[floor] = FALSE;
        EXIT_CRITICAL();

        if (up_req) {
            if ((Floor_t)floor < FLOOR_1 || (Floor_t)floor > FLOOR_4) {
                Usart1_TransmitString("DISPATCH: ERR invalid floor\r\n");
                continue;
            }
            if (!Is_Call_Pending((Floor_t)floor, CALL_DIR_UP)) {
                Add_Pending_Call((Floor_t)floor, CALL_DIR_UP);
            }
        }
    }

    /* DOWN calls: floors 2, 3, 4 */
    for (uint8 floor = 2; floor <= 4; floor++) {
        ENTER_CRITICAL();
        boolean down_req = g_HallwayDownRequests[floor];
        if (down_req) g_HallwayDownRequests[floor] = FALSE;
        EXIT_CRITICAL();

        if (down_req) {
            if ((Floor_t)floor < FLOOR_1 || (Floor_t)floor > FLOOR_4) {
                Usart1_TransmitString("DISPATCH: ERR invalid floor\r\n");
                continue;
            }
            if (!Is_Call_Pending((Floor_t)floor, CALL_DIR_DOWN)) {
                Add_Pending_Call((Floor_t)floor, CALL_DIR_DOWN);
            }
        }
    }

    /* ── Process unassigned pending calls ── */
    for (uint8 i = 0; i < s_call_count; i++) {
        if (!s_calls[i].active) continue;
        if (s_calls[i].assigned) continue; /* Already dispatched */

        if (use_comm_fault) {
            /* COMM FAULT: all calls → Elevator A */
            LiftState_t a_state = ElevatorFSM_GetState(s_fsm_a);
            if (a_state != LIFT_STATE_EMERGENCY) {
                Dispatcher_AssignToA(s_calls[i].floor);
                s_calls[i].assigned    = TRUE;
                s_calls[i].assigned_to = 0;
            }
            /* If A is also in emergency, leave call pending */
        } else {
            Process_Call(s_calls[i].floor, s_calls[i].direction);
            /* Mark as assigned after Process_Call handles it */
        }
    }

    /* ── Garbage-collect completed calls ── */
    for (uint8 i = 0; i < s_call_count; i++) {
        if (!s_calls[i].active) continue;
        if (!s_calls[i].assigned) continue;

        /* Check if the assigned elevator has served this floor */
        if (s_calls[i].assigned_to == 0) {
            /* Elevator A */
            LiftState_t st = ElevatorFSM_GetState(s_fsm_a);
            Floor_t cur = ElevatorFSM_GetCurrentFloor(s_fsm_a);
            if (cur == s_calls[i].floor &&
                (st == LIFT_STATE_DOORS_OPEN || st == LIFT_STATE_IDLE)) {
                s_calls[i].active = FALSE;
            }
        } else {
            /* Elevator B — check from IPC */
            ENTER_CRITICAL();
            uint8 b_cur = g_rx_packet.current_floor;
            uint8 b_st  = g_rx_packet.lift_state;
            EXIT_CRITICAL();
            if ((Floor_t)b_cur == s_calls[i].floor &&
                (b_st == (uint8)LIFT_STATE_DOORS_OPEN ||
                 b_st == (uint8)LIFT_STATE_IDLE)) {
                s_calls[i].active = FALSE;
            }
        }
    }

    /* Compact the call table */
    uint8 write = 0;
    for (uint8 read = 0; read < s_call_count; read++) {
        if (s_calls[read].active) {
            if (write != read) {
                s_calls[write] = s_calls[read];
            }
            write++;
        }
    }
    s_call_count = write;
}

/* ================================================================
 * Dispatcher_AssignToA
 * ================================================================ */
void Dispatcher_AssignToA(Floor_t floor) {
    if (s_fsm_a == (void*)0) return;
    ElevatorFSM_AssignTarget(s_fsm_a, floor);

    Usart1_TransmitString("DISPATCH: A -> floor ");
    SafePrintFloor(floor);
    Usart1_TransmitString("\r\n");
}

/* ================================================================
 * Dispatcher_PublishSlaveTarget
 * ================================================================ */
void Dispatcher_PublishSlaveTarget(Floor_t floor) {
    if (floor == s_last_slave_target) return;

    ENTER_CRITICAL();
    g_tx_packet.target_floor = (uint8)floor;
    EXIT_CRITICAL();

    s_last_slave_target = floor;

    Usart1_TransmitString("DISPATCH: B -> floor ");
    SafePrintFloor(floor);
    Usart1_TransmitString("\r\n");
}

/* ================================================================
 * Dispatcher_ReassignCalls — reassign calls from a failed elevator
 * ================================================================ */
void Dispatcher_ReassignCalls(uint8 elevator_id) {
    for (uint8 i = 0; i < s_call_count; i++) {
        if (!s_calls[i].active) continue;
        if (!s_calls[i].assigned) continue;
        if (s_calls[i].assigned_to != elevator_id) continue;

        /* Un-assign so it gets re-processed next tick */
        s_calls[i].assigned = FALSE;
    }
}

/* ================================================================
 * Process_Call — run priority algorithm for one call
 * ================================================================ */
static void Process_Call(Floor_t call_floor, CallDirection_t dir) {
    /* Snapshot Elevator A (local) */
    LiftState_t a_state   = ElevatorFSM_GetState(s_fsm_a);
    Floor_t     a_current = ElevatorFSM_GetCurrentFloor(s_fsm_a);
    Floor_t     a_target  = ElevatorFSM_GetTargetFloor(s_fsm_a);

    /* Snapshot Elevator B (from IPC) */
    ENTER_CRITICAL();
    LiftState_t b_state   = (LiftState_t)g_rx_packet.lift_state;
    Floor_t     b_current = (Floor_t)g_rx_packet.current_floor;
    Floor_t     b_target  = (Floor_t)g_rx_packet.target_floor;
    EXIT_CRITICAL();

    /* Score both */
    uint8 score_a = Score_Elevator(a_state, a_current, a_target, call_floor, dir);
    uint8 score_b = Score_Elevator(b_state, b_current, b_target, call_floor, dir);

    /* Emergency exclusion */
    if (a_state == LIFT_STATE_EMERGENCY) score_a = DISPATCH_SCORE_UNAVAILABLE;
    if (b_state == LIFT_STATE_EMERGENCY) score_b = DISPATCH_SCORE_UNAVAILABLE;

    /* Both unavailable — leave call pending */
    if (score_a == DISPATCH_SCORE_UNAVAILABLE &&
        score_b == DISPATCH_SCORE_UNAVAILABLE) {
        return;
    }

    /* Tie-break: closer wins; further tie: prefer A */
    boolean assign_to_a;
    if (score_a > score_b) {
        assign_to_a = TRUE;
    } else if (score_b > score_a) {
        assign_to_a = FALSE;
    } else {
        uint8 dist_a = Floor_Distance(a_current, call_floor);
        uint8 dist_b = Floor_Distance(b_current, call_floor);
        assign_to_a = (dist_a <= dist_b) ? TRUE : FALSE; /* Prefer A on tie */
    }

    /* Assign and mark in pending table */
    for (uint8 i = 0; i < s_call_count; i++) {
        if (!s_calls[i].active) continue;
        if (s_calls[i].floor == call_floor && s_calls[i].direction == dir) {
            if (assign_to_a) {
                Dispatcher_AssignToA(call_floor);
                s_calls[i].assigned    = TRUE;
                s_calls[i].assigned_to = 0;
            } else {
                Dispatcher_PublishSlaveTarget(call_floor);
                s_calls[i].assigned    = TRUE;
                s_calls[i].assigned_to = 1;
            }
            break;
        }
    }
}

/* ================================================================
 * Score_Elevator — priority scoring for one elevator vs one call
 * ================================================================ */
static uint8 Score_Elevator(LiftState_t state, Floor_t current,
                            Floor_t target, Floor_t call_floor,
                            CallDirection_t call_dir) {

    /* EMERGENCY — cannot accept */
    if (state == LIFT_STATE_EMERGENCY) {
        return DISPATCH_SCORE_UNAVAILABLE;
    }

    /* IMMEDIATE: idle and at the call floor */
    if ((state == LIFT_STATE_IDLE || state == LIFT_STATE_DOORS_OPEN) &&
        current == call_floor) {
        return DISPATCH_SCORE_IMMEDIATE;
    }

    /* PERFECT MATCH: moving toward call_floor in same direction, not passed */
    if (state == LIFT_STATE_MOVING_UP && call_dir == CALL_DIR_UP) {
        if ((uint8)current < (uint8)call_floor &&
            (uint8)call_floor <= (uint8)target) {
            return DISPATCH_SCORE_PERFECT_MATCH;
        }
    }
    if (state == LIFT_STATE_MOVING_DOWN && call_dir == CALL_DIR_DOWN) {
        if ((uint8)current > (uint8)call_floor &&
            (uint8)call_floor >= (uint8)target) {
            return DISPATCH_SCORE_PERFECT_MATCH;
        }
    }

    /* PASSED MATCH: same direction but already passed */
    if (state == LIFT_STATE_MOVING_UP && call_dir == CALL_DIR_UP) {
        if ((uint8)call_floor <= (uint8)current) {
            return DISPATCH_SCORE_PASSED_MATCH;
        }
    }
    if (state == LIFT_STATE_MOVING_DOWN && call_dir == CALL_DIR_DOWN) {
        if ((uint8)call_floor >= (uint8)current) {
            return DISPATCH_SCORE_PASSED_MATCH;
        }
    }

    /* OPPOSITE DIR: moving away — exclude from consideration */
    if ((state == LIFT_STATE_MOVING_UP   && call_dir == CALL_DIR_DOWN) ||
        (state == LIFT_STATE_MOVING_DOWN && call_dir == CALL_DIR_UP)) {
        return DISPATCH_SCORE_OPPOSITE_DIR;
    }

    /* IDLE NEAREST: idle but not at floor */
    if (state == LIFT_STATE_IDLE || state == LIFT_STATE_DOORS_OPEN) {
        return DISPATCH_SCORE_IDLE_NEAREST;
    }

    return DISPATCH_SCORE_UNAVAILABLE;
}

/* ================================================================
 * Utilities
 * ================================================================ */
static uint8 Floor_Distance(Floor_t a, Floor_t b) {
    uint8 ua = (uint8)a;
    uint8 ub = (uint8)b;
    return (ua > ub) ? (ua - ub) : (ub - ua);
}

static boolean Is_Call_Pending(Floor_t floor, CallDirection_t dir) {
    for (uint8 i = 0; i < s_call_count; i++) {
        if (s_calls[i].active &&
            s_calls[i].floor == floor &&
            s_calls[i].direction == dir) {
            return TRUE;
        }
    }
    return FALSE;
}

static void Add_Pending_Call(Floor_t floor, CallDirection_t dir) {
    if (s_call_count >= MAX_PENDING_CALLS) {
        Usart1_TransmitString("DISPATCH: WARN call table full\r\n");
        return;
    }
    s_calls[s_call_count].floor       = floor;
    s_calls[s_call_count].direction   = dir;
    s_calls[s_call_count].active      = TRUE;
    s_calls[s_call_count].assigned    = FALSE;
    s_calls[s_call_count].assigned_to = 0;
    s_call_count++;
}

static void SafePrintFloor(Floor_t floor) {
    uint8 f = (uint8)floor;
    if (f >= 1 && f <= NUM_FLOORS) {
        Usart1_TransmitByte((uint8)('0' + f));
    } else {
        Usart1_TransmitByte((uint8)'?');
    }
}

#endif /* BUILD_AS_MASTER */
