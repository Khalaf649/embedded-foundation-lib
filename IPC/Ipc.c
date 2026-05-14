//
// Ipc.c
// Inter-Processor Communication layer over SPI.
//
// How it works (one 50ms cycle):
//
//  1. SEAL   — disable IRQs, stamp header, compute checksum, re-enable.
//  2. SEND   — hand the sealed TX buffer to the SPI driver (non-blocking).
//  3. VALIDATE — once SPI goes IDLE, XOR-check the received packet.
//  4. FAULT  — track consecutive failures; set g_comm_fault after 5 misses.
//

#include "Ipc.h"
#include "../Nvic/Nvic.h"   // ENTER_CRITICAL() / EXIT_CRITICAL()

// =========================================================
// Globals
// =========================================================
volatile Ipc_Packet_t g_tx_packet;
volatile Ipc_Packet_t g_rx_packet;
volatile boolean        g_comm_fault = FALSE;

// =========================================================
// Private
// =========================================================
static uint8 s_timeout_counter    = 0;
static boolean s_transfer_pending   = FALSE;   // 1 after a transfer is launched,
                                          // 0 until the first Sync call

// =========================================================
// Ipc_CalculateChecksum  (XOR of bytes 0..6)
// =========================================================
uint8 Ipc_CalculateChecksum(volatile Ipc_Packet_t* packet) {
    uint8 xor_sum = 0;
    volatile uint8* ptr = (volatile uint8*)packet;

    for (uint8 i = 0; i < (IPC_PACKET_SIZE - 1U); i++) {
        xor_sum ^= ptr[i];
    }
    return xor_sum;
}

// =========================================================
// Ipc_ValidatePacket
// =========================================================
boolean Ipc_ValidatePacket(volatile Ipc_Packet_t* packet) {
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

    // TX packet — safe defaults, will be overwritten each sync cycle
    g_tx_packet.header        = IPC_HEADER_BYTE;
    g_tx_packet.current_floor = FLOOR_1;          // Start at floor 1
    g_tx_packet.target_floor  = FLOOR_NONE;       // No target initially
    g_tx_packet.lift_state    = LIFT_STATE_IDLE;  // Not moving
    g_tx_packet.motor_speed   = MOTOR_REST; // 0% PWM
    g_tx_packet.emergency     = EMERGENCY_NORMAL; // Safe state
    g_tx_packet.cabin_buttons = CABIN_BTN_NONE;   // No buttons pressed
    g_tx_packet.checksum      = 0U;               // Will be computed dynamically

    // RX packet — fully zeroed (prevents stale stack garbage)
    g_rx_packet.header        = 0x00U;
    g_rx_packet.current_floor = FLOOR_NONE;
    g_rx_packet.target_floor  = FLOOR_NONE;
    g_rx_packet.lift_state    = LIFT_STATE_IDLE;
    g_rx_packet.motor_speed   = MOTOR_REST;
    g_rx_packet.emergency     = EMERGENCY_NORMAL;
    g_rx_packet.cabin_buttons = CABIN_BTN_NONE;
    g_rx_packet.checksum      = 0U;

    g_comm_fault       = FALSE; // Using the boolean FALSE
    s_timeout_counter  = 0U;
    s_transfer_pending = 0U;

    EXIT_CRITICAL();
}

// =========================================================
// Ipc_Sync  — called every 50ms by the scheduler
// =========================================================
void Ipc_Sync(void) {

    /* ----------------------------------------------------------
     * STEP 1 — VALIDATE previous cycle's received data
     *
     * We validate BEFORE launching the new transfer so we are
     * always acting on the packet that fully arrived last cycle,
     * not the one that just started streaming in.
     *
     * Guard: skip on the very first call (no transfer has happened
     * yet, so g_rx_packet is still zeroed from Ipc_Init).
     * ---------------------------------------------------------- */
    if (s_transfer_pending) {
        if (Spi1_GetState() == SPI_STATUS_IDLE) {
            if (Ipc_ValidatePacket(&g_rx_packet)) {
                // Good packet — reset fault counter
                s_timeout_counter = 0U;
                g_comm_fault      = FALSE;
            } else {
                // Bad packet — sanitise dangerous fields
                s_timeout_counter++;

                ENTER_CRITICAL();
                g_rx_packet.target_floor  = FLOOR_NONE;       // ignore corrupted target
                g_rx_packet.lift_state    = LIFT_STATE_IDLE;  // assume IDLE (safe)
                g_rx_packet.emergency     = EMERGENCY_NORMAL; // don't inject false emergencies
                EXIT_CRITICAL();

                if (s_timeout_counter >= IPC_TIMEOUT_LIMIT) {
                    g_comm_fault = TRUE;
                    // Do NOT reset s_timeout_counter here; keep it saturated
                    // so g_comm_fault stays set until a good packet arrives.
                }
            }
        }
        // If SPI is still BUSY here the previous transfer hasn't
        // finished — skip launching a new one this cycle.
    }

    /* ----------------------------------------------------------
     * STEP 2 — SEAL the TX packet for this cycle
     *
     * Critical section prevents the Elevator FSM from writing a
     * field between the checksum computation and the SPI dispatch.
     * ---------------------------------------------------------- */
    ENTER_CRITICAL();
    g_tx_packet.header   = IPC_HEADER_BYTE;
    g_tx_packet.checksum = Ipc_CalculateChecksum(&g_tx_packet);
    EXIT_CRITICAL();

    /* ----------------------------------------------------------
     * STEP 3 — DISPATCH
     *
     * Only launch if SPI is free (don't clobber a running transfer).
     * ---------------------------------------------------------- */
    if (Spi1_GetState() == SPI_STATUS_IDLE) {

#ifdef BUILD_AS_MASTER
        Spi1_Master_Start_Exchange(
            (volatile uint8*)&g_tx_packet,
            (volatile uint8*)&g_rx_packet,
            IPC_PACKET_SIZE
        );
#else
        Spi1_Slave_Stage_Data(
            (volatile uint8*)&g_tx_packet,
            (volatile uint8*)&g_rx_packet,
            IPC_PACKET_SIZE
        );
#endif
        s_transfer_pending = TRUE;
    }
    // If SPI was busy we simply skip this cycle's dispatch;
    // the previous packet will be validated on the next call.
}