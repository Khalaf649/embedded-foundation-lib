//
// Ipc.c
// Inter-Processor Communication layer over SPI.
//
// How it works (one 50ms cycle):
//
//  1. SEAL   — disable IRQs, snapshot g_tx_packet to s_spi_tx_buffer, stamp
//  header, compute checksum.
//  2. SEND   — hand the sealed TX buffer to the SPI driver (non-blocking).
//  3. VALIDATE — once SPI goes IDLE, XOR-check the received s_spi_rx_buffer.
//  4. COMMIT — if valid, copy s_spi_rx_buffer to g_rx_packet under critical
//  section.
//  5. FAULT  — track consecutive failures; set g_comm_fault after 5 misses.
//
// Bug fixes applied:
//  FIX-A: Transfer timeout — force-reset SPI if stuck BUSY for too long
//  FIX-B: Boot grace period — skip fault counting during startup
//  FIX-C: Slave immediate re-stage — prepare next TX right after commit
//

#include "Ipc.h"
#include "../Nvic/Nvic.h" // ENTER_CRITICAL() / EXIT_CRITICAL()
#include "../Scheduler/Scheduler.h" // Scheduler_GetTick()

// =========================================================
// Globals
// =========================================================
volatile Ipc_Packet_t g_tx_packet;
volatile Ipc_Packet_t g_rx_packet;
volatile boolean g_comm_fault = FALSE;

// =========================================================
// Private (Double-buffering to prevent FSM race conditions)
// These MUST be volatile as they are modified in SPI ISR.
// =========================================================
static volatile Ipc_Packet_t s_spi_tx_buffer;
static volatile Ipc_Packet_t s_spi_rx_buffer;
static uint8 s_timeout_counter = 0;
static boolean s_transfer_pending = FALSE;
static uint32 s_transfer_start_tick = 0;  // FIX-A: When the current transfer started
static uint8 s_boot_grace_counter = 0;    // FIX-B: Boot grace countdown

// =========================================================
// Forward declaration for internal helper
// =========================================================
static void Ipc_SealAndDispatch(void);

// =========================================================
// Ipc_CalculateChecksum  (XOR of bytes 0..6)
// =========================================================
uint8 Ipc_CalculateChecksum(volatile Ipc_Packet_t *packet) {
  uint8 xor_sum = 0;
  volatile uint8 *ptr = (volatile uint8 *)packet;

  for (uint8 i = 0; i < (IPC_PACKET_SIZE - 1U); i++) {
    xor_sum ^= ptr[i];
  }
  return xor_sum;
}

// =========================================================
// Ipc_ValidatePacket
// =========================================================
boolean Ipc_ValidatePacket(volatile Ipc_Packet_t *packet) {
  // Rule 1: magic header
  if (packet->header != IPC_HEADER_BYTE) {
    return FALSE;
  }
  // Rule 2: XOR checksum
  if (packet->checksum != Ipc_CalculateChecksum(packet)) {
    return FALSE;
  }
  return TRUE;
}

// =========================================================
// Ipc_Init
// =========================================================
void Ipc_Init(void) {
  ENTER_CRITICAL();

  // TX packet — safe defaults
  g_tx_packet.header = IPC_HEADER_BYTE;
  g_tx_packet.current_floor = (uint8)FLOOR_1;
  g_tx_packet.target_floor = (uint8)FLOOR_NONE;
  g_tx_packet.lift_state = (uint8)LIFT_STATE_IDLE;
  g_tx_packet.motor_speed = (uint8)MOTOR_REST;
  g_tx_packet.emergency = (uint8)EMERGENCY_NORMAL;
  g_tx_packet.cabin_buttons = CABIN_BTN_NONE;
  g_tx_packet.checksum = 0U;

  // RX packet — fully zeroed
  g_rx_packet.header = 0x00U;
  g_rx_packet.current_floor = (uint8)FLOOR_NONE;
  g_rx_packet.target_floor = (uint8)FLOOR_NONE;
  g_rx_packet.lift_state = (uint8)LIFT_STATE_IDLE;
  g_rx_packet.motor_speed = (uint8)MOTOR_REST;
  g_rx_packet.emergency = (uint8)EMERGENCY_NORMAL;
  g_rx_packet.cabin_buttons = CABIN_BTN_NONE;
  g_rx_packet.checksum = 0U;

  // Clear SPI buffers
  volatile uint8 *tx_ptr = (volatile uint8 *)&s_spi_tx_buffer;
  volatile uint8 *rx_ptr = (volatile uint8 *)&s_spi_rx_buffer;
  for (uint8 i = 0; i < IPC_PACKET_SIZE; i++) {
    tx_ptr[i] = 0;
    rx_ptr[i] = 0;
  }

  g_comm_fault = FALSE;
  s_timeout_counter = 0U;
  s_transfer_pending = FALSE;
  s_transfer_start_tick = 0U;
  s_boot_grace_counter = IPC_BOOT_GRACE_CYCLES; // FIX-B: Grace period at boot

  EXIT_CRITICAL();
}

// =========================================================
// Ipc_SealAndDispatch — prepare TX buffer and start SPI
// Extracted to a helper so both the normal path and the
// slave re-stage path can share the same logic.
// =========================================================
static void Ipc_SealAndDispatch(void) {
  if (Spi1_GetState() != SPI_STATUS_IDLE) return;

  ENTER_CRITICAL();
  // Snapshot g_tx_packet to the SPI buffer byte-by-byte
  volatile uint8 *src = (volatile uint8 *)&g_tx_packet;
  volatile uint8 *dst = (volatile uint8 *)&s_spi_tx_buffer;
  for (uint8 i = 0; i < IPC_PACKET_SIZE; i++) {
    dst[i] = src[i];
  }
  EXIT_CRITICAL();

  // Finalize header and checksum on the stable snapshot
  s_spi_tx_buffer.header = IPC_HEADER_BYTE;
  s_spi_tx_buffer.checksum = Ipc_CalculateChecksum(&s_spi_tx_buffer);

  // Dispatch to SPI driver
#ifdef BUILD_AS_MASTER
  Spi1_Master_Start_Exchange((volatile uint8 *)&s_spi_tx_buffer,
                             (volatile uint8 *)&s_spi_rx_buffer,
                             IPC_PACKET_SIZE);
#else
  Spi1_Slave_Stage_Data((volatile uint8 *)&s_spi_tx_buffer,
                        (volatile uint8 *)&s_spi_rx_buffer, IPC_PACKET_SIZE);
#endif

  s_transfer_pending = TRUE;
  s_transfer_start_tick = Scheduler_GetTick();
}

// =========================================================
// Ipc_Sync  — called every 50ms by the scheduler
// =========================================================
void Ipc_Sync(void) {

  /* ----------------------------------------------------------
   * STEP 0 — TRANSFER TIMEOUT CHECK (FIX-A)
   *
   * If the SPI has been BUSY for longer than IPC_TRANSFER_TIMEOUT_MS,
   * the transfer is stuck (e.g., slave waiting for master clock that
   * never came, or master waiting for RXNE that never fired due to OVR).
   * Force-reset SPI so we can try again.
   * ---------------------------------------------------------- */
  if (s_transfer_pending && Spi1_GetState() == SPI_STATUS_BUSY) {
    uint32 now = Scheduler_GetTick();
    uint32 elapsed = now - s_transfer_start_tick;
    if (elapsed >= IPC_TRANSFER_TIMEOUT_MS) {
      /* Force-reset the SPI peripheral and driver state */
      Spi1_ForceReset();
      s_transfer_pending = FALSE;

      /* Count this as a failed transfer */
      if (s_boot_grace_counter == 0) {
        s_timeout_counter++;
        if (s_timeout_counter >= IPC_TIMEOUT_LIMIT) {
          g_comm_fault = TRUE;
        }
      }
    }
  }

  /* ----------------------------------------------------------
   * STEP 1 — VALIDATE previous cycle's received data
   * ---------------------------------------------------------- */
  if (s_transfer_pending) {
    if (Spi1_GetState() == SPI_STATUS_IDLE) {
      if (Ipc_ValidatePacket(&s_spi_rx_buffer)) {
        // Good packet — commit to global state byte-by-byte
        ENTER_CRITICAL();
        volatile uint8 *src = (volatile uint8 *)&s_spi_rx_buffer;
        volatile uint8 *dst = (volatile uint8 *)&g_rx_packet;
        for (uint8 i = 0; i < IPC_PACKET_SIZE; i++) {
          dst[i] = src[i];
        }
        EXIT_CRITICAL();

        s_timeout_counter = 0U;
        g_comm_fault = FALSE;
      } else {
        // Bad packet — track failures (but not during boot grace)
        if (s_boot_grace_counter == 0) {
          s_timeout_counter++;

          if (s_timeout_counter >= IPC_TIMEOUT_LIMIT) {
            g_comm_fault = TRUE;
            // On comm fault, reset RX state to safe defaults
            ENTER_CRITICAL();
            g_rx_packet.target_floor = (uint8)FLOOR_NONE;
            g_rx_packet.lift_state = (uint8)LIFT_STATE_IDLE;
            g_rx_packet.emergency = (uint8)EMERGENCY_NORMAL;
            EXIT_CRITICAL();
          }
        }
      }
      s_transfer_pending = FALSE;
    }
  }

  /* ----------------------------------------------------------
   * STEP 1.5 — BOOT GRACE COUNTDOWN (FIX-B)
   *
   * During the first IPC_BOOT_GRACE_CYCLES calls, we don't
   * count failures. This gives both boards time to boot,
   * initialize SPI, and stage their first packets.
   * ---------------------------------------------------------- */
  if (s_boot_grace_counter > 0) {
    s_boot_grace_counter--;
  }

  /* ----------------------------------------------------------
   * STEP 2 — SEAL & DISPATCH the TX packet for this cycle
   * ---------------------------------------------------------- */
  if (!s_transfer_pending) {
    Ipc_SealAndDispatch();
  }
}