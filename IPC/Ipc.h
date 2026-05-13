//
// Created by zramd on 13/05/2026.
//

#ifndef IPC_H
#define IPC_H

#include "../include/STD_TYPES.h"
#include "stm32f4xx.h"

#define IPC_HEADER_BYTE 0xA5

typedef enum {
    ELEV_IDLE = 0,
    ELEV_MOVING_UP,
    ELEV_MOVING_DOWN,
    ELEV_DOORS_OPEN,
    ELEV_EMERGENCY
} ElevState_t;

// 8-Byte Fixed Frame
typedef struct {
    uint8 header;       // Always 0xA5
    uint8 state;        // ElevState_t
    uint8 current_floor;// 1 to 4
    uint8 target_floor; // 1 to 4
    uint8 door_status;  // 0=Closed, 1=Open
    uint8 dummy1;       // Reserved
    uint8 dummy2;       // Reserved
    uint8 checksum;     // XOR of bytes 0 to 6
} Ipc_Packet_t;

// Critical Section Macros
#define ENTER_CRITICAL()  __disable_irq()
#define EXIT_CRITICAL()   __enable_irq()

uint8 IPC_CalculateChecksum(Ipc_Packet_t* packet);

#endif