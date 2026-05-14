/*
 * App.c — Full application orchestration (rewritten from scratch)
 *
 * ═══════════════════════════════════════════════════════════════════
 * CORNER CASES EXPLICITLY HANDLED
 * ═══════════════════════════════════════════════════════════════════
 *  - Emergency flag checked every FSM tick (10ms), pre-empts all
 *  - Emergency reset via g_Emergency_Flag clear + FSM reset API
 *  - INDEPENDENT_MODE on comm fault (FSM handles internally)
 *  - Slave pre-stages IPC TX before first Master clock burst
 *  - Floor values clamped to [1..4] before any UART print
 *  - PWM ramp driven by timer-based task (50ms), not blocking delay
 *  - Scheduler cooperative: no task may block (except UART polling)
 *
 * ═══════════════════════════════════════════════════════════════════
 * VOLATILE FLAGS DECLARED / USED
 * ═══════════════════════════════════════════════════════════════════
 *  - g_CabinRequests[5]         (Button.h) — consumed by FSM_Tick
 *  - g_Current_Floor_Sensor     (Button.h) — consumed by FSM_Tick
 *  - g_Emergency_Flag           (Button.h) — consumed by FSM_Tick
 *  - g_HallwayUpRequests[5]     (Button.h) — consumed by Dispatcher
 *  - g_HallwayDownRequests[5]   (Button.h) — consumed by Dispatcher
 *  - g_tx_packet, g_rx_packet   (Ipc.h)    — SPI exchange buffers
 *  - g_comm_fault               (Ipc.h)    — comm fault flag
 *
 * ═══════════════════════════════════════════════════════════════════
 * DRIVER API ASSUMPTIONS
 * ═══════════════════════════════════════════════════════════════════
 *  - Usart1_Init(), Usart1_TransmitString(), Usart1_TransmitByte()
 *  - Spi1_Init() configures SPI based on BUILD_AS_MASTER
 *  - Button_Init() sets up all EXTI callbacks + NVIC
 *  - Motor_Init() configures PWM timer + GPIO
 *  - Ipc_Init(), Ipc_Sync() handle packet seal/dispatch/validate
 *  - Scheduler_Init(), Scheduler_RegisterTask(), Scheduler_Run()
 *  - Scheduler_GetTick() returns ms tick counter
 *  - ElevatorFSM_Init/Tick/RampTick/ResetEmergency/Get* accessors
 *  - Dispatcher_Init/Tick (Master only)
 *
 * ═══════════════════════════════════════════════════════════════════
 * SCHEDULER TASK PERIODS
 * ═══════════════════════════════════════════════════════════════════
 *  - FSM tick     :  10 ms
 *  - IPC sync     :  50 ms
 *  - PWM ramp     :  50 ms
 *  - Dispatcher   :  10 ms (Master only)
 *  - Telemetry    : 500 ms (UART polling OK)
 */

#include "App.h"
#include "../ElevatorFSM/ElevatorFSM.h"
#include "../Dispatcher/Dispatcher.h"
#include "../IPC/Ipc.h"
#include "../SPI/Spi.h"
#include "../Button/Button.h"
#include "../Motor/Motor.h"
#include "../Scheduler/Scheduler.h"
#include "../Usart/Usart.h"
#include "../Nvic/Nvic.h"

/* ================================================================
 * Module-level FSM instance
 * ================================================================ */
static ElevatorFSM_t s_fsm_local;  /* This board's elevator */

/* ================================================================
 * Scheduler task slot indices
 * ================================================================ */
static uint8 s_task_fsm_idx       = 0xFF;
static uint8 s_task_ipc_idx       = 0xFF;
static uint8 s_task_ramp_idx      = 0xFF;
static uint8 s_task_telemetry_idx = 0xFF;
#ifdef BUILD_AS_MASTER
static uint8 s_task_dispatcher_idx = 0xFF;
#endif

/* ================================================================
 * Forward declarations — task functions
 * ================================================================ */
static void Task_FsmTick(void);
static void Task_IpcSync(void);
static void Task_PwmRamp(void);
static void Task_Telemetry(void);
#ifdef BUILD_AS_MASTER
static void Task_Dispatcher(void);
#endif
static void Telemetry_Report(void);

/* ── Telemetry helpers ── */
static void Tx_SafeFloor(uint8 val);
static void Tx_Speed(Motor_Speed_t spd);
static void Tx_State(LiftState_t state);

/* ================================================================
 *  ═══════════════════════════════════════════════════════
 *                   Board A — MASTER
 *  ═══════════════════════════════════════════════════════
 * ================================================================ */
#ifdef BUILD_AS_MASTER

void App_Init(void) {
    /* 1. USART — debug output first */
    Usart1_Init();
    Usart1_TransmitString("\r\n=== ELEVATOR MASTER BOOT ===\r\n");

    /* 2. SPI — hardware ready before IPC */
    Spi1_Init();
    Usart1_TransmitString("SPI1 init (Master)\r\n");

    /* 3. Buttons / EXTI — cabin + hallway + sensors + emergency */
    Button_Init();
    Usart1_TransmitString("Buttons/EXTI init\r\n");

    /* 4. Motor — PWM + GPIO, speed = 0 */
    Motor_Init();
    Usart1_TransmitString("Motor init\r\n");

    /* 5. IPC — zero packets */
    Ipc_Init();
    Usart1_TransmitString("IPC init\r\n");

    /* 6. Elevator FSM (Elevator A) */
    ElevatorFSM_Init(&s_fsm_local);
    Usart1_TransmitString("Elevator A FSM init\r\n");

    /* 7. Dispatcher — gets pointer to local FSM */
    Dispatcher_Init(&s_fsm_local);
    Usart1_TransmitString("Dispatcher init\r\n");

    /* 8. Scheduler — register all tasks */
    Scheduler_Init();

    s_task_fsm_idx        = Scheduler_RegisterTask(Task_FsmTick,     10U);
    s_task_ipc_idx        = Scheduler_RegisterTask(Task_IpcSync,     50U);
    s_task_ramp_idx       = Scheduler_RegisterTask(Task_PwmRamp,     50U);
    s_task_dispatcher_idx = Scheduler_RegisterTask(Task_Dispatcher,  10U);
    s_task_telemetry_idx  = Scheduler_RegisterTask(Task_Telemetry,  500U);

    Usart1_TransmitString("Scheduler started\r\n");
    Usart1_TransmitString("=== MASTER READY ===\r\n\r\n");
}

void App_Run(void) {
    while (1) {
        Scheduler_Run();
    }
}

/* ── Master task implementations ── */

static void Task_FsmTick(void) {
    ElevatorFSM_Tick(&s_fsm_local);
}

static void Task_IpcSync(void) {
    Ipc_Sync();
}

static void Task_PwmRamp(void) {
    ElevatorFSM_RampTick(&s_fsm_local);
}

static void Task_Dispatcher(void) {
    Dispatcher_Tick();
}

static void Task_Telemetry(void) {
    Telemetry_Report();
}

#endif /* BUILD_AS_MASTER */

/* ================================================================
 *  ═══════════════════════════════════════════════════════
 *                   Board B — SLAVE
 *  ═══════════════════════════════════════════════════════
 * ================================================================ */
#ifndef BUILD_AS_MASTER

void App_Slave_Init(void) {
    /* 1. USART */
    Usart1_Init();
    Usart1_TransmitString("\r\n=== ELEVATOR SLAVE BOOT ===\r\n");

    /* 2. SPI — slave mode */
    Spi1_Init();
    Usart1_TransmitString("SPI1 init (Slave)\r\n");

    /* 3. Buttons / EXTI — cabin + sensors + emergency (no hallway) */
    Button_Init();
    Usart1_TransmitString("Buttons/EXTI init\r\n");

    /* 4. Motor */
    Motor_Init();
    Usart1_TransmitString("Motor init\r\n");

    /* 5. IPC — pre-stage TX so first Master clock gets valid data */
    Ipc_Init();
    Ipc_Sync();  /* Arms the slave DR immediately */
    Usart1_TransmitString("IPC init\r\n");

    /* 6. Elevator FSM (Elevator B) */
    ElevatorFSM_Init(&s_fsm_local);
    Usart1_TransmitString("Elevator B FSM init\r\n");

    /* 7. Scheduler — no Dispatcher on Slave */
    Scheduler_Init();

    s_task_fsm_idx       = Scheduler_RegisterTask(Task_FsmTick,    10U);
    s_task_ipc_idx       = Scheduler_RegisterTask(Task_IpcSync,    50U);
    s_task_ramp_idx      = Scheduler_RegisterTask(Task_PwmRamp,    50U);
    s_task_telemetry_idx = Scheduler_RegisterTask(Task_Telemetry, 500U);

    Usart1_TransmitString("Scheduler started\r\n");
    Usart1_TransmitString("=== SLAVE READY ===\r\n\r\n");
}

void App_Slave_Run(void) {
    while (1) {
        Scheduler_Run();
    }
}

/* ── Slave task implementations ── */

static void Task_FsmTick(void) {
    ElevatorFSM_Tick(&s_fsm_local);
}

static void Task_IpcSync(void) {
    Ipc_Sync();
}

static void Task_PwmRamp(void) {
    ElevatorFSM_RampTick(&s_fsm_local);
}

static void Task_Telemetry(void) {
    Telemetry_Report();
}

#endif /* !BUILD_AS_MASTER */

/* ================================================================
 * Telemetry — 500ms UART status dump
 *
 * CRITICAL: All floor values are clamped to [1..4] before printing.
 * If a value is out of range, '?' is printed and an error logged.
 * This prevents the "Floor 9" bug from raw sensor/EXTI pin numbers.
 * ================================================================ */

/** Print a floor value safely — clamp to [1..4] or print '?' */
static void Tx_SafeFloor(uint8 val) {
    if (val >= 1 && val <= NUM_FLOORS) {
        Usart1_TransmitByte((uint8)('0' + val));
    } else if (val == 0) {
        Usart1_TransmitByte((uint8)'-');  /* FLOOR_NONE */
    } else {
        Usart1_TransmitByte((uint8)'?');
        Usart1_TransmitString("[ERR]");
    }
}

static void Tx_Speed(Motor_Speed_t spd) {
    switch (spd) {
        case MOTOR_REST:       Usart1_TransmitString("0%");   break;
        case MOTOR_LOW_SPEED:  Usart1_TransmitString("20%");  break;
        case MOTOR_HIGH_SPEED: Usart1_TransmitString("100%"); break;
        default:               Usart1_TransmitString("?%");   break;
    }
}

static void Tx_State(LiftState_t state) {
    switch (state) {
        case LIFT_STATE_IDLE:        Usart1_TransmitString("IDLE      "); break;
        case LIFT_STATE_MOVING_UP:   Usart1_TransmitString("MOVING_UP "); break;
        case LIFT_STATE_MOVING_DOWN: Usart1_TransmitString("MOVING_DN "); break;
        case LIFT_STATE_DOORS_OPEN:  Usart1_TransmitString("DOORS_OPN "); break;
        case LIFT_STATE_EMERGENCY:   Usart1_TransmitString("EMERGENCY "); break;
        case LIFT_STATE_INDEPENDENT: Usart1_TransmitString("INDEPENDNT"); break;
        default:                     Usart1_TransmitString("UNKNOWN   "); break;
    }
}

static void Telemetry_Report(void) {
#ifdef BUILD_AS_MASTER
    /* ── Elevator A (local) ── */
    Usart1_TransmitString("[A] F:");
    Tx_SafeFloor((uint8)ElevatorFSM_GetCurrentFloor(&s_fsm_local));
    Usart1_TransmitString(" T:");
    Tx_SafeFloor((uint8)ElevatorFSM_GetTargetFloor(&s_fsm_local));
    Usart1_TransmitString(" S:");
    Tx_State(ElevatorFSM_GetState(&s_fsm_local));
    Usart1_TransmitString(" M:");
    Tx_Speed(Motor_GetSpeed());
    Usart1_TransmitString(" CF:");
    Usart1_TransmitByte(g_comm_fault ? '1' : '0');
    Usart1_TransmitString("\r\n");

    /* ── Elevator B (from IPC) ── */
    ENTER_CRITICAL();
    uint8 b_floor = g_rx_packet.current_floor;
    uint8 b_tgt   = g_rx_packet.target_floor;
    uint8 b_state = g_rx_packet.lift_state;
    uint8 b_motor = g_rx_packet.motor_speed;
    EXIT_CRITICAL();

    Usart1_TransmitString("[B] F:");
    Tx_SafeFloor(b_floor);
    Usart1_TransmitString(" T:");
    Tx_SafeFloor(b_tgt);
    Usart1_TransmitString(" S:");
    Tx_State((LiftState_t)b_state);
    Usart1_TransmitString(" M:");
    Tx_Speed((Motor_Speed_t)b_motor);
    Usart1_TransmitString("\r\n");

#else
    /* ── Slave: report own state ── */
    Usart1_TransmitString("[B] F:");
    Tx_SafeFloor((uint8)ElevatorFSM_GetCurrentFloor(&s_fsm_local));
    Usart1_TransmitString(" T:");
    Tx_SafeFloor((uint8)ElevatorFSM_GetTargetFloor(&s_fsm_local));
    Usart1_TransmitString(" S:");
    Tx_State(ElevatorFSM_GetState(&s_fsm_local));
    Usart1_TransmitString(" M:");
    Tx_Speed(Motor_GetSpeed());
    Usart1_TransmitString(" CF:");
    Usart1_TransmitByte(g_comm_fault ? '1' : '0');
    Usart1_TransmitString("\r\n");
#endif
}
