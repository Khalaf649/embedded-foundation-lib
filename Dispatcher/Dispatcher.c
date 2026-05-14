//
// Dispatcher.c
// Master-only hallway call dispatcher.
//
// Each hallway call is a (floor, direction) pair.
// For every pending call the Dispatcher scores both elevators using the
// priority table from the project spec, then assigns the higher-scoring one.
//
// Score function for one elevator against one call:
//
//   IMMEDIATE     : elevator.current == call.floor && elevator.state == IDLE
//   PERFECT_MATCH : elevator is moving TOWARD call.floor in the SAME direction
//                   and has NOT yet passed it
//   PASSED_MATCH  : elevator is moving in the SAME direction but already passed floor
//   IDLE          : elevator is IDLE (not at floor — that is IMMEDIATE)
//   OPPOSITE_DIR  : elevator is moving AWAY from the call direction
//   NO_MATCH      : elevator is in EMERGENCY — cannot be assigned
//
// Tie-breaking: when both elevators have the same score, choose the one
// that is physically closer (fewer floors away) to minimise wait time.
//

#ifdef BUILD_AS_MASTER

#include "Dispatcher.h"
#include "../Ipc/Ipc.h"
#include "../Button/Button.h"
#include "../Nvic/Nvic.h"   /* ENTER_CRITICAL / EXIT_CRITICAL */
#include "../Usart/Usart.h"

/* ================================================================
 * Private state
 * ================================================================ */
static ElevatorFSM_t *s_fsm_a = NULL;   /* Elevator A — local (Master board)  */
static ElevatorFSM_t *s_fsm_b = NULL;   /* Elevator B — remote (Slave board)  */

/* Shadow of the last target we sent to Slave via IPC.
 * Prevents re-sending the same assignment every tick. */
static Floor_t s_last_slave_target = FLOOR_NONE;

/* ================================================================
 * Private helpers — forward declarations
 * ================================================================ */
static uint8   Dispatcher_Score(LiftState_t state, Floor_t current,
                                Floor_t target,    Floor_t call_floor,
                                CallDirection_t call_dir);
static uint8   Dispatcher_Distance(Floor_t a, Floor_t b);
static void    Dispatcher_ProcessCall(Floor_t call_floor, CallDirection_t dir);
static boolean Dispatcher_ElevatorAvailable(LiftState_t state);

/* ================================================================
 * Dispatcher_Init
 * ================================================================ */
void Dispatcher_Init(ElevatorFSM_t *fsm_a, ElevatorFSM_t *fsm_b) {
    s_fsm_a            = fsm_a;
    s_fsm_b            = fsm_b;
    s_last_slave_target = FLOOR_NONE;
}

/* ================================================================
 * Dispatcher_Tick — called every 50 ms by the scheduler
 *
 * Scans all six hallway buttons (U1, U2, D2, U3, D3, D4).
 * For each pending call, runs the priority algorithm and dispatches.
 * ================================================================ */
void Dispatcher_Tick(void) {
    /* ── Comm Fault mode ──
     * Slave has gone silent. Master takes ALL hallway calls for itself.
     * Slave should have entered independent mode (it detects comm fault
     * via its own IPC timeout counter and serves cabin-only requests). */
    if (g_comm_fault) {
        /* Process all hallway calls on Master only */
        for (uint8 floor = 1; floor <= 4; floor++) {
            boolean up_req, down_req;

            ENTER_CRITICAL();
            up_req   = g_HallwayUpRequests[floor];
            down_req = g_HallwayDownRequests[floor];
            if (up_req)   g_HallwayUpRequests[floor]   = FALSE;
            if (down_req) g_HallwayDownRequests[floor]  = FALSE;
            EXIT_CRITICAL();

            if (up_req)   Dispatcher_AssignToA((Floor_t)floor);
            if (down_req) Dispatcher_AssignToA((Floor_t)floor);
        }
        return;
    }

    /* ── Normal mode ──
     * Read Slave's current state from g_rx_packet (refreshed by Ipc_Sync). */
    LiftState_t slave_state   = (LiftState_t)g_rx_packet.lift_state;
    Floor_t     slave_current = (Floor_t)g_rx_packet.current_floor;
    Floor_t     slave_target  = (Floor_t)g_rx_packet.target_floor;

    /* Slave in emergency — do not assign to it */
    if (slave_state == LIFT_STATE_EMERGENCY ||
        g_rx_packet.emergency == EMERGENCY_STOP) {
        /* Degrade: all calls go to Master's elevator */
        for (uint8 floor = 1; floor <= 4; floor++) {
            boolean up_req, down_req;
            ENTER_CRITICAL();
            up_req   = g_HallwayUpRequests[floor];
            down_req = g_HallwayDownRequests[floor];
            if (up_req)   g_HallwayUpRequests[floor]   = FALSE;
            if (down_req) g_HallwayDownRequests[floor]  = FALSE;
            EXIT_CRITICAL();
            if (up_req)   Dispatcher_AssignToA((Floor_t)floor);
            if (down_req) Dispatcher_AssignToA((Floor_t)floor);
        }
        return;
    }

    /* ── Process each hallway call independently ── */
    /* Hallway calls that exist:
     *   UP:   floor 1, 2, 3  (cannot go UP from floor 4)
     *   DOWN: floor 2, 3, 4  (cannot go DOWN from floor 1)
     */

    /* UP calls */
    for (uint8 floor = 1; floor <= 3; floor++) {
        boolean up_req;
        ENTER_CRITICAL();
        up_req = g_HallwayUpRequests[floor];
        if (up_req) g_HallwayUpRequests[floor] = FALSE;
        EXIT_CRITICAL();
        if (up_req) {
            Dispatcher_ProcessCall((Floor_t)floor, CALL_DIR_UP);
        }
    }

    /* DOWN calls */
    for (uint8 floor = 2; floor <= 4; floor++) {
        boolean down_req;
        ENTER_CRITICAL();
        down_req = g_HallwayDownRequests[floor];
        if (down_req) g_HallwayDownRequests[floor] = FALSE;
        EXIT_CRITICAL();
        if (down_req) {
            Dispatcher_ProcessCall((Floor_t)floor, CALL_DIR_DOWN);
        }
    }

    /* ── Suppress slave target change — only send when it changes ── */
    (void)slave_state;
    (void)slave_current;
    (void)slave_target;
}

/* ================================================================
 * Dispatcher_AssignToA  — assign a floor to the local elevator
 * ================================================================ */
void Dispatcher_AssignToA(Floor_t floor) {
    ElevatorFSM_AssignTarget(s_fsm_a, floor);
}

/* ================================================================
 * Dispatcher_PublishSlaveTarget
 * Write the assignment for Elevator B into g_tx_packet.
 * IPC Sync will transmit it to the Slave on the next 50 ms cycle.
 * ================================================================ */
void Dispatcher_PublishSlaveTarget(Floor_t floor) {
    if (floor == s_last_slave_target) return; /* Nothing new to send */

    ENTER_CRITICAL();
    g_tx_packet.target_floor = (uint8)floor;
    EXIT_CRITICAL();

    s_last_slave_target = floor;

    Usart1_TransmitString("DISPATCH: Slave → floor ");
    char buf[4] = {'0' + (char)floor, '\r', '\n', '\0'};
    Usart1_TransmitString(buf);
}

/* ================================================================
 * Dispatcher_ProcessCall — run priority algorithm for one call
 * ================================================================ */
static void Dispatcher_ProcessCall(Floor_t call_floor, CallDirection_t dir) {
    /* Snapshot Elevator A (local FSM) */
    LiftState_t a_state   = ElevatorFSM_GetState(s_fsm_a);
    Floor_t     a_current = ElevatorFSM_GetCurrentFloor(s_fsm_a);
    Floor_t     a_target  = ElevatorFSM_GetTargetFloor(s_fsm_a);

    /* Snapshot Elevator B (from last valid IPC rx packet) */
    LiftState_t b_state   = (LiftState_t)g_rx_packet.lift_state;
    Floor_t     b_current = (Floor_t)g_rx_packet.current_floor;
    Floor_t     b_target  = (Floor_t)g_rx_packet.target_floor;

    /* Score both elevators */
    uint8 score_a = Dispatcher_Score(a_state, a_current, a_target,
                                     call_floor, dir);
    uint8 score_b = Dispatcher_Score(b_state, b_current, b_target,
                                     call_floor, dir);

    /* Do not assign to an unavailable elevator */
    boolean a_ok = Dispatcher_ElevatorAvailable(a_state);
    boolean b_ok = Dispatcher_ElevatorAvailable(b_state);

    if (!a_ok) score_a = DISPATCH_SCORE_NO_MATCH;
    if (!b_ok) score_b = DISPATCH_SCORE_NO_MATCH;

    /* Both unavailable — nothing to do */
    if (score_a == DISPATCH_SCORE_NO_MATCH &&
        score_b == DISPATCH_SCORE_NO_MATCH) {
        Usart1_TransmitString("DISPATCH: no elevator available\r\n");
        return;
    }

    /* Tie-breaking: closer elevator wins when scores are equal */
    boolean assign_to_a;
    if (score_a > score_b) {
        assign_to_a = TRUE;
    } else if (score_b > score_a) {
        assign_to_a = FALSE;
    } else {
        /* Equal score — pick closest */
        uint8 dist_a = Dispatcher_Distance(a_current, call_floor);
        uint8 dist_b = Dispatcher_Distance(b_current, call_floor);
        assign_to_a = (dist_a <= dist_b);
    }

    if (assign_to_a) {
        Usart1_TransmitString("DISPATCH: Elevator A → floor ");
        char buf[4] = {'0' + (char)call_floor, '\r', '\n', '\0'};
        Usart1_TransmitString(buf);
        Dispatcher_AssignToA(call_floor);
    } else {
        Dispatcher_PublishSlaveTarget(call_floor);
    }
}

/* ================================================================
 * Dispatcher_Score
 *
 * Scores a single elevator (defined by state/current/target) against
 * a specific call (call_floor, call_dir) using the mandated algorithm.
 * ================================================================ */
static uint8 Dispatcher_Score(LiftState_t state,   Floor_t current,
                               Floor_t     target,   Floor_t call_floor,
                               CallDirection_t call_dir) {
    /* Rule 1 — IMMEDIATE: already at floor and idle */
    if (state == LIFT_STATE_IDLE && current == call_floor) {
        return DISPATCH_SCORE_IMMEDIATE;
    }

    /* Rule 2 — PERFECT MATCH:
     *   - Elevator is moving in the same direction as the call.
     *   - The call floor is between current and target (not yet passed). */
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

    /* Rule 3 — PASSED MATCH:
     *   Moving in the same direction but the call floor is BEHIND current. */
    if (state == LIFT_STATE_MOVING_UP && call_dir == CALL_DIR_UP) {
        if ((uint8)call_floor < (uint8)current) {
            return DISPATCH_SCORE_PASSED_MATCH;
        }
    }
    if (state == LIFT_STATE_MOVING_DOWN && call_dir == CALL_DIR_DOWN) {
        if ((uint8)call_floor > (uint8)current) {
            return DISPATCH_SCORE_PASSED_MATCH;
        }
    }

    /* Rule 4 — IDLE: available but not at the called floor */
    if (state == LIFT_STATE_IDLE) {
        return DISPATCH_SCORE_IDLE;
    }

    /* Rule 5 — OPPOSITE DIR: moving away from the call's direction.
     *   Do NOT assign yet; give lowest non-zero score so it becomes a
     *   candidate only when no better option exists. */
    if ((state == LIFT_STATE_MOVING_UP   && call_dir == CALL_DIR_DOWN) ||
        (state == LIFT_STATE_MOVING_DOWN && call_dir == CALL_DIR_UP)) {
        return DISPATCH_SCORE_OPPOSITE_DIR;
    }

    /* Doors open — treat similarly to idle for scoring */
    if (state == LIFT_STATE_DOORS_OPEN) {
        return DISPATCH_SCORE_IDLE;
    }

    return DISPATCH_SCORE_NO_MATCH;
}

/* Floor distance (absolute) */
static uint8 Dispatcher_Distance(Floor_t a, Floor_t b) {
    uint8 ua = (uint8)a;
    uint8 ub = (uint8)b;
    return (ua > ub) ? (ua - ub) : (ub - ua);
}

/* Returns TRUE if this elevator can accept a new assignment */
static boolean Dispatcher_ElevatorAvailable(LiftState_t state) {
    return (state != LIFT_STATE_EMERGENCY);
}

#endif /* BUILD_AS_MASTER */
