//
// Created by zramd on 13/05/2026.
//

#ifndef SPI_H
#define SPI_H

#include "../include/STD_TYPES.h"
#include "../IPC/Ipc.h"

extern volatile uint8 spi_transfer_complete;
extern Ipc_Packet_t rx_packet;
extern Ipc_Packet_t tx_packet; // <-- ADD THIS LINE

void Spi2_Init_Master(void);
void Spi2_Init_Slave(void);
void Spi2_Start_Exchange(Ipc_Packet_t* local_data);

#endif