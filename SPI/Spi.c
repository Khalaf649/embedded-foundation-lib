//
// Created by Khalaf on [Current Date].
//

#include "Spi.h"

#include <stddef.h>

#include "../GPIO/GPIO.h"
#include "../Nvic/Nvic.h" // FIX 1: Added missing include for ENTER/EXIT_CRITICAL and Nvic_EnableInterrupt
#include "../RCC/RCC.h"
#include "stm32f4xx.h"
#include "../include/BIT_MATH.h"

/* ==================================================================== */
/* Private State Variables                                              */
/* ==================================================================== */
static volatile uint8* spi_tx_ptr    = NULL;
static volatile uint8* spi_rx_ptr    = NULL;
static volatile uint16  spi_length    = 0;
static volatile uint16  spi_byte_index = 0;
static volatile SPI_Status_t spi_state = SPI_STATUS_IDLE;

/* ==================================================================== */
/* Hardware Pins Setup                                                  */
/* ==================================================================== */
static GPIO_Pin_Location_t sck_pin  = {GPIO_PORT_A, GPIO_PIN_5};
static GPIO_Pin_Location_t miso_pin = {GPIO_PORT_A, GPIO_PIN_6};
static GPIO_Pin_Location_t mosi_pin = {GPIO_PORT_A, GPIO_PIN_7};
static GPIO_Pin_Location_t nss_pin  = {GPIO_PORT_A, GPIO_PIN_4};

/* ==================================================================== */
/* Core Functions                                                       */
/* ==================================================================== */

void Spi1_Init(void) {

    /* 1. Enable SPI1 clock: SPI1 is on APB2 (Bit 12) -> ID = (3 * 32) + 12 = 108 */
    RCC_EnablePeripheral(108);

    /* 2. Configure SCK, MISO, MOSI as Alternate Function (AF5) */
    GPIO_PinConfig_t spi_pins_cfg = {
        .mode  = GPIO_MODE_AF,
        .pull  = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH,
        .otype = GPIO_OTYPE_PP
    };

    GPIO_InitPin(&sck_pin,  &spi_pins_cfg);
    GPIO_InitPin(&miso_pin, &spi_pins_cfg);
    GPIO_InitPin(&mosi_pin, &spi_pins_cfg);

    GPIO_SetAlternateFunction(&sck_pin,  GPIO_AF_5);
    GPIO_SetAlternateFunction(&miso_pin, GPIO_AF_5);
    GPIO_SetAlternateFunction(&mosi_pin, GPIO_AF_5);

    /* 3. Configure Master or Slave specific settings */
#ifdef BUILD_AS_MASTER
    /* Master controls NSS/CS manually via software */
    GPIO_PinConfig_t nss_master_cfg = {
        .mode  = GPIO_MODE_OUTPUT,
        .pull  = GPIO_PULL_NONE,
        .speed = GPIO_SPEED_HIGH,
        .otype = GPIO_OTYPE_PP
    };
    GPIO_InitPin(&nss_pin, &nss_master_cfg);
    GPIO_WritePin(&nss_pin, GPIO_HIGH); // Default Idle State: CS deasserted

    /* SPI1 Master Config */
    SET_BIT(SPI1->CR1, 2); // MSTR: Master configuration
    SET_BIT(SPI1->CR1, 8); // SSI:  Internal slave select (keeps NSS high internally)
    SET_BIT(SPI1->CR1, 9); // SSM:  Software slave management

    /* Set BaudRate to fPCLK/16 (BR[2:0] = 011) */
    SET_BIT(SPI1->CR1, 3);
    SET_BIT(SPI1->CR1, 4);
    CLR_BIT(SPI1->CR1, 5);

    /* FIX 2: Explicitly set SPI Mode 0 (CPOL=0, CPHA=0) so both boards are documented to match */
    CLR_BIT(SPI1->CR1, 0); // CPHA = 0
    CLR_BIT(SPI1->CR1, 1); // CPOL = 0

#else
    /* Slave uses Hardware NSS for immediate CS detection */
    GPIO_InitPin(&nss_pin, &spi_pins_cfg);
    GPIO_SetAlternateFunction(&nss_pin, GPIO_AF_5);

    /* SPI1 Slave Config: explicit reset ensures clean state, MSTR=0, SSM=0, Mode 0 */
    SPI1->CR1 = 0; // CPOL=0, CPHA=0, MSTR=0 — all correct by reset value
#endif

    /* 4. Enable RXNE interrupt and SPI peripheral */
    SET_BIT(SPI1->CR2, 6); // RXNEIE: RX buffer not empty interrupt enable
    SET_BIT(SPI1->CR1, 6); // SPE:    SPI peripheral enable

    /* 5. Enable SPI1 IRQ in NVIC */
    Nvic_EnableInterrupt(SPI1_IRQn);
}

/* ==================================================================== */
/* Master / Slave Transfer Functions                                    */
/* ==================================================================== */

#ifdef BUILD_AS_MASTER
void Spi1_Master_Start_Exchange(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len) {
    /* FIX 3: Parameters are volatile uint8* to match the volatile packet buffers,
     * eliminating the implicit volatile-drop cast warning. */
    if (spi_state == SPI_STATUS_BUSY) return; // Prevent overlapping transfers

    ENTER_CRITICAL();
    spi_tx_ptr    = tx_buf;
    spi_rx_ptr    = rx_buf;
    spi_length    = len;
    spi_byte_index = 0;
    spi_state     = SPI_STATUS_BUSY;

    GPIO_WritePin(&nss_pin, GPIO_LOW); // Assert CS to wake up slave
    EXIT_CRITICAL();

    SPI1->DR = spi_tx_ptr[0]; // Write first byte to trigger the clock
}
#else
void Spi1_Slave_Stage_Data(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len) {
    /* FIX 3: Same volatile parameter fix as Master above. */

    /* FIX 4: Guard against re-staging while a transfer is still in progress.
     * Without this, a fast 50ms tick could corrupt spi_byte_index mid-transfer. */
    if (spi_state == SPI_STATUS_BUSY) return;

    ENTER_CRITICAL();
    spi_tx_ptr    = tx_buf;
    spi_rx_ptr    = rx_buf;
    spi_length    = len;
    spi_byte_index = 0;
    spi_state     = SPI_STATUS_BUSY;

    /* THE SLAVE CHALLENGE: Pre-load DR before the Master's clock arrives */
    SPI1->DR = spi_tx_ptr[0];
    EXIT_CRITICAL();
}
#endif

SPI_Status_t Spi1_GetState(void) {
    return spi_state;
}

/* ==================================================================== */
/* Interrupt Handler                                                    */
/* ==================================================================== */

void SPI1_IRQHandler(void) {
    /* Check RXNE flag (SR bit 0): data has arrived from the other board */
    if (GET_BIT(SPI1->SR, 0)) {

        /* 1. Read the received byte — this also clears the RXNE flag */
        spi_rx_ptr[spi_byte_index] = SPI1->DR; // we must read to make the rxne false
        spi_byte_index++;

        /* 2. Send next byte or finalize */
        if (spi_byte_index < spi_length) {
            /* FIX 5: Check TXE (SR bit 1) before writing DR to avoid
             * overrunning the TX buffer at higher clock speeds. */
            if (GET_BIT(SPI1->SR, 1)) {
                SPI1->DR = spi_tx_ptr[spi_byte_index];
            }
        } else {
            /* Transfer complete */
            spi_state = SPI_STATUS_IDLE;

#ifdef BUILD_AS_MASTER
            /* Master deasserts CS to terminate the transaction */
            GPIO_WritePin(&nss_pin, GPIO_HIGH);
#endif
        }
    }
}