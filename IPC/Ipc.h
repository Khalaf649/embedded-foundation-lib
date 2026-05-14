//
// Ipc.h
// Inter-Processor Communication layer over SPI.
//
// Packet layout (8 bytes, fixed-length):
//
//  Byte  Field           Notes
//  ----  --------------  -------------------------------------------
//   0    header          Always 0xA5  (IPC_HEADER_BYTE)
//   1    current_floor   1..4  — where this elevator is right now
//   2    target_floor    1..4  — where this elevator is heading (0 = none)
//   3    lift_state      Elevator_State_t cast to uint8
//   4    motor_speed     Motor_Speed_t cast to uint8
//   5    emergency       1 = emergency active, 0 = normal
//   6    cabin_buttons   Bitmask: bit N = floor N+1 requested (bits 0-3)
//   7    checksum        XOR of bytes 0..6
//

#ifndef IPC_H
#define IPC_H

#include "STD_TYPES.h"
#include "../Spi/Spi.h"
#include "../Motor/Motor.h"

// =========================================================
// Constants
// =========================================================
#define IPC_HEADER_BYTE     0xA5U
#define IPC_PACKET_SIZE     8U

// Number of consecutive failed validations before declaring a comm fault
#define IPC_TIMEOUT_LIMIT   5U   // 5 × 50ms = 250ms










// =========================================================
// Packet struct
// Must be exactly IPC_PACKET_SIZE bytes — verified by static assert below.
// =========================================================
typedef struct {
    uint8 header;           // Byte 0
    uint8 current_floor;    // Byte 1
    uint8 target_floor;     // Byte 2
    uint8 lift_state;       // Byte 3
    uint8 motor_speed;      // Byte 4
    uint8 emergency;        // Byte 5
    uint8 cabin_buttons;    // Byte 6
    uint8 checksum;         // Byte 7  (XOR of bytes 0..6)
} Ipc_Packet_t;



// =========================================================
// Shared packet instances (written by FSM, read by Ipc_Sync)
// =========================================================
extern volatile Ipc_Packet_t g_tx_packet;   // Our state → other board
extern volatile Ipc_Packet_t g_rx_packet;   // Other board's state → us

// =========================================================
// Comm-fault flag (set by Ipc_Sync after IPC_TIMEOUT_LIMIT failures)
// Read by the Dispatcher and Elevator FSM.
// =========================================================
extern volatile boolean g_comm_fault;

// =========================================================
// API
// =========================================================

/** One-time init: zero both packets, set safe TX defaults. */
void  Ipc_Init(void);

/** Called every 50ms by the scheduler. Seals the TX packet,
 *  dispatches the SPI exchange, validates the received packet,
 *  and updates g_comm_fault. */
void  Ipc_Sync(void);

/** Returns 1 if the packet passes header and checksum checks. */
uint8 Ipc_ValidatePacket(volatile Ipc_Packet_t* packet);

/** Returns the XOR checksum of bytes 0..6. */
uint8 Ipc_CalculateChecksum(volatile Ipc_Packet_t* packet);

#endif // IPC_H