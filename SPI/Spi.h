//
// Spi.h — SPI1 Driver for Full-Duplex Master/Slave Communication
//

#ifndef SPI_H
#define SPI_H

#include "STD_TYPES.h"
#include "../Nvic/Nvic.h" // ENTER_CRITICAL AND EXIT_CRITICAL

/* --- SPI Driver Status --- */
typedef enum {
    SPI_STATUS_IDLE = 0,
    SPI_STATUS_BUSY = 1
} SPI_Status_t;

/* --- Core APIs --- */
void Spi1_Init(void);

#ifdef BUILD_AS_MASTER
/* Master Mode: Initiates the transfer and controls CS */
void Spi1_Master_Start_Exchange(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len);
#else
/* Slave Mode: Stages data into DR before clock arrives */
void Spi1_Slave_Stage_Data(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len);
#endif

/* Check if the background interrupt transfer is done */
SPI_Status_t Spi1_GetState(void);

/* Force-reset the SPI state machine (used by IPC timeout recovery) */
void Spi1_ForceReset(void);

#endif // SPI_H