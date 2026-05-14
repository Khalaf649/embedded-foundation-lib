/*
 * App.c — Application orchestration (Diagnosing Sync Issue)
 */

#include "App.h"
#include "../Button/Button.h"
#include "../Dispatcher/Dispatcher.h"
#include "../ElevatorFSM/ElevatorFSM.h"
#include "../FloorLED/FloorLED.h"
#include "../IPC/Ipc.h"
#include "../Motor/Motor.h"
#include "../Nvic/Nvic.h"
#include "../SPI/Spi.h"
#include "../Scheduler/Scheduler.h"
#include "../Usart/Usart.h"

static ElevatorFSM_t s_fsm_local;

static uint8 s_task_fsm_idx = 0xFF;
static uint8 s_task_ipc_idx = 0xFF;
static uint8 s_task_telemetry_idx = 0xFF;
#ifdef BUILD_AS_MASTER
static uint8 s_task_dispatcher_idx = 0xFF;
#endif

static void Task_FsmTick(void);
static void Task_IpcSync(void);
static void Task_Telemetry(void);
#ifdef BUILD_AS_MASTER
static void Task_Dispatcher(void);
#endif
static void Telemetry_Report(void);

static void Tx_SafeFloor(uint8 val);
static void Tx_Speed(Motor_Speed_t spd);
static void Tx_State(LiftState_t state);

#ifdef BUILD_AS_MASTER
void App_Init(void) {
  Usart1_Init();
  Usart1_TransmitString("\r\n=== ELEVATOR MASTER BOOT ===\r\n");
  Spi1_Init();
  Button_Init();
  Motor_Init();
  FloorLED_Init();
  Ipc_Init();
  ElevatorFSM_Init(&s_fsm_local);
  Dispatcher_Init(&s_fsm_local);
  Scheduler_Init();
  s_task_fsm_idx = Scheduler_RegisterTask(Task_FsmTick, 10U);
  s_task_ipc_idx = Scheduler_RegisterTask(Task_IpcSync, 50U);
  s_task_dispatcher_idx = Scheduler_RegisterTask(Task_Dispatcher, 10U);
  s_task_telemetry_idx = Scheduler_RegisterTask(Task_Telemetry, 500U);
  Usart1_TransmitString("=== MASTER READY ===\r\n\r\n");
}
void App_Run(void) {
  while (1)
    Scheduler_Run();
}
static void Task_FsmTick(void) {
  ElevatorFSM_Tick(&s_fsm_local);
  FloorLED_Update(ElevatorFSM_GetCurrentFloor(&s_fsm_local));
}
static void Task_IpcSync(void) { Ipc_Sync(); }
static void Task_Dispatcher(void) { Dispatcher_Tick(); }
static void Task_Telemetry(void) { Telemetry_Report(); }
#endif

#ifndef BUILD_AS_MASTER
void App_Slave_Init(void) {
  Usart1_Init();
  Usart1_TransmitString("\r\n=== ELEVATOR SLAVE BOOT ===\r\n");
  Spi1_Init();
  Button_Init();
  Motor_Init();
  FloorLED_Init();
  Ipc_Init();
  /* NOTE: Do NOT call Ipc_Sync() here. The scheduler will trigger
   * the first sync at the proper time, ensuring the slave stages
   * its data only when it is ready to receive the master's clock. */
  ElevatorFSM_Init(&s_fsm_local);
  Scheduler_Init();
  s_task_fsm_idx = Scheduler_RegisterTask(Task_FsmTick, 10U);
  s_task_ipc_idx = Scheduler_RegisterTask(Task_IpcSync, 50U);
  s_task_telemetry_idx = Scheduler_RegisterTask(Task_Telemetry, 500U);
  Usart1_TransmitString("=== SLAVE READY ===\r\n\r\n");
}
void App_Slave_Run(void) {
  while (1)
    Scheduler_Run();
}
static void Task_FsmTick(void) {
  ElevatorFSM_Tick(&s_fsm_local);
  FloorLED_Update(ElevatorFSM_GetCurrentFloor(&s_fsm_local));
}
static void Task_IpcSync(void) { Ipc_Sync(); }
static void Task_Telemetry(void) { Telemetry_Report(); }
#endif

static void Tx_SafeFloor(uint8 val) {
  if (val >= 1 && val <= NUM_FLOORS)
    Usart1_TransmitByte((uint8)('0' + val));
  else if (val == 0)
    Usart1_TransmitByte((uint8)'-');
  else {
    Usart1_TransmitByte((uint8)'?');
    Usart1_TransmitString("[E]");
  }
}
static void Tx_Speed(Motor_Speed_t spd) {
  switch (spd) {
  case MOTOR_REST:
    Usart1_TransmitString("0%");
    break;
  case MOTOR_LOW_SPEED:
    Usart1_TransmitString("20%");
    break;
  case MOTOR_HIGH_SPEED:
    Usart1_TransmitString("100%");
    break;
  default:
    Usart1_TransmitString("?%");
    break;
  }
}
static void Tx_State(LiftState_t state) {
  switch (state) {
  case LIFT_STATE_IDLE:
    Usart1_TransmitString("IDLE      ");
    break;
  case LIFT_STATE_MOVING_UP:
    Usart1_TransmitString("MOVING_UP ");
    break;
  case LIFT_STATE_MOVING_DOWN:
    Usart1_TransmitString("MOVING_DN ");
    break;
  case LIFT_STATE_DOORS_OPEN:
    Usart1_TransmitString("DOORS_OPN ");
    break;
  case LIFT_STATE_EMERGENCY:
    Usart1_TransmitString("EMERGENCY ");
    break;
  case LIFT_STATE_INDEPENDENT:
    Usart1_TransmitString(" ");
    break;
  default:
    Usart1_TransmitString("UNKNOWN   ");
    break;
  }
}

static void Telemetry_Report(void) {
  /* Always report Local Elevator First */
#ifdef BUILD_AS_MASTER
  Usart1_TransmitString("[A] F:");
#else
  Usart1_TransmitString("[B] F:");
#endif
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

  /* Report Remote Elevator */
  ENTER_CRITICAL();
  uint8 r_floor = g_rx_packet.current_floor;
  uint8 r_tgt = g_rx_packet.target_floor;
  uint8 r_state = g_rx_packet.lift_state;
  uint8 r_motor = g_rx_packet.motor_speed;
  EXIT_CRITICAL();

#ifdef BUILD_AS_MASTER
  Usart1_TransmitString("[B] F:");
#else
  Usart1_TransmitString("[A] F:");
#endif
  Tx_SafeFloor(r_floor);
  Usart1_TransmitString(" T:");
  Tx_SafeFloor(r_tgt);
  Usart1_TransmitString(" S:");
  Tx_State((LiftState_t)r_state);
  Usart1_TransmitString(" M:");
  Tx_Speed((Motor_Speed_t)r_motor);
  Usart1_TransmitString("\r\n");
}
