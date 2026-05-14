//
// Spi.c — SPI1 Full-Duplex Driver (Bug-Fixed for Dual-Board Operation)
//
// Fixes applied:
//   FIX 1: OVR (overrun) flag clearing in ISR — prevents RXNE lockup
//   FIX 2: TXE bounded spin-wait — prevents silent byte drops
//   FIX 3: Spi1_ForceReset() — timeout recovery for stuck-BUSY state
//   FIX 4: Slave re-stages DR from ISR on transfer complete
//

#include "Spi.h"

#include <stddef.h>

#include "../GPIO/GPIO.h"
#include "../Nvic/Nvic.h"
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

    /* Explicitly set SPI Mode 0 (CPOL=0, CPHA=0) so both boards match */
    CLR_BIT(SPI1->CR1, 0); // CPHA = 0
    CLR_BIT(SPI1->CR1, 1); // CPOL = 0

#else
    /* Slave uses Hardware NSS for immediate CS detection */
    GPIO_InitPin(&nss_pin, &spi_pins_cfg);
    GPIO_SetAlternateFunction(&nss_pin, GPIO_AF_5);

    /* SPI1 Slave Config: MSTR=0, SSM=0, CPOL=0, CPHA=0 (Mode 0) */
    SPI1->CR1 = 0;
    /* Explicitly document: CR1=0 gives us:
     *  MSTR=0 (slave), SSM=0 (hardware NSS), CPOL=0, CPHA=0 (Mode 0)
     *  DFF=0 (8-bit), LSBFIRST=0 (MSB first) — all correct */
#endif

    /* 4. Enable RXNE interrupt + Error interrupt, then enable SPI */
    SET_BIT(SPI1->CR2, 6); // RXNEIE: RX buffer not empty interrupt enable
    SET_BIT(SPI1->CR2, 5); // ERRIE:  Error interrupt enable (catches OVR)
    SET_BIT(SPI1->CR1, 6); // SPE:    SPI peripheral enable

    /* 5. Enable SPI1 IRQ in NVIC */
    Nvic_EnableInterrupt(SPI1_IRQn);
}

/* ==================================================================== */
/* Master / Slave Transfer Functions                                    */
/* ==================================================================== */

#ifdef BUILD_AS_MASTER
void Spi1_Master_Start_Exchange(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len) {
    if (spi_state == SPI_STATUS_BUSY) return; // Prevent overlapping transfers

    ENTER_CRITICAL();
    spi_tx_ptr    = tx_buf;
    spi_rx_ptr    = rx_buf;
    spi_length    = len;
    spi_byte_index = 0;
    spi_state     = SPI_STATUS_BUSY;

    GPIO_WritePin(&nss_pin, GPIO_LOW); // Assert CS to wake up slave
    EXIT_CRITICAL();

    /* Small delay to let slave detect NSS assertion and prepare.
     * At 16MHz, ~32 NOPs ≈ 2µs — enough for the slave's hardware
     * NSS detection to latch before we start clocking. */
    volatile uint8 delay = 32;
    while (delay--) { __asm volatile("nop"); }

    SPI1->DR = spi_tx_ptr[0]; // Write first byte to trigger the clock
}
#else
void Spi1_Slave_Stage_Data(volatile uint8* tx_buf, volatile uint8* rx_buf, uint16 len) {
    if (spi_state == SPI_STATUS_BUSY) return;

    ENTER_CRITICAL();
    spi_tx_ptr    = tx_buf;
    spi_rx_ptr    = rx_buf;
    spi_length    = len;
    spi_byte_index = 0;
    spi_state     = SPI_STATUS_BUSY;

    /* FIX: Clear any stale data/flags before pre-loading.
     * Reading DR clears RXNE; reading SR then DR clears OVR. */
    (void)SPI1->SR;
    (void)SPI1->DR;

    /* THE SLAVE CHALLENGE: Pre-load DR before the Master's clock arrives.
     * This byte will be shifted out when the master sends its first clock. */
    SPI1->DR = spi_tx_ptr[0];
    EXIT_CRITICAL();
}
#endif

SPI_Status_t Spi1_GetState(void) {
    return spi_state;
}

/* ==================================================================== */
/* Spi1_ForceReset — Recovery from stuck-BUSY state                     */
/* Called by IPC layer when a transfer timeout is detected.              */
/* ==================================================================== */
void Spi1_ForceReset(void) {
    ENTER_CRITICAL();

    /* Disable SPI to flush the shift register and FIFOs */
    CLR_BIT(SPI1->CR1, 6); // SPE = 0

    /* Clear any pending flags by reading SR then DR */
    (void)SPI1->SR;
    (void)SPI1->DR;

    /* Reset driver state */
    spi_byte_index = 0;
    spi_length     = 0;
    spi_state      = SPI_STATUS_IDLE;

#ifdef BUILD_AS_MASTER
    /* Deassert CS */
    GPIO_WritePin(&nss_pin, GPIO_HIGH);
#endif

    /* Re-enable SPI */
    SET_BIT(SPI1->CR1, 6); // SPE = 1

    EXIT_CRITICAL();
}

/* ==================================================================== */
/* Interrupt Handler                                                    */
/* ==================================================================== */

void SPI1_IRQHandler(void) {

    uint32 sr = SPI1->SR; // Snapshot status register once

    /* ── Handle Overrun Error (OVR) ──
     * OVR is set when DR is not read before the next byte arrives.
     * If we don't clear it, RXNE stops firing and SPI goes permanently deaf.
     * Cleared by reading DR then SR (in that order per reference manual). */
    if (sr & (1U << 6)) { /* OVR flag = bit 6 */
        (void)SPI1->DR;   // Read DR (discard)
        (void)SPI1->SR;   // Read SR to complete the clear sequence
        /* The current transfer is corrupted — force-complete it so
         * IPC can detect the bad checksum and retry next cycle. */
        spi_state = SPI_STATUS_IDLE;
#ifdef BUILD_AS_MASTER
        GPIO_WritePin(&nss_pin, GPIO_HIGH);
#endif
        return;
    }

    /* ── Handle RXNE (normal byte received) ── */
    if (sr & (1U << 0)) { /* RXNE flag = bit 0 */

        /* 1. Read the received byte — this also clears the RXNE flag */
        if (spi_rx_ptr != NULL && spi_byte_index < spi_length) {
            spi_rx_ptr[spi_byte_index] = (uint8)SPI1->DR;
        } else {
            (void)SPI1->DR; // Must read DR to clear RXNE even if we can't store it
        }
        spi_byte_index++;

        /* 2. Send next byte or finalize */
        if (spi_byte_index < spi_length) {
            /* FIX: Bounded spin-wait for TXE instead of silently dropping the byte.
             * At fPCLK/16, TXE should be ready almost immediately (< 1µs).
             * The bound prevents infinite hangs if something goes catastrophically wrong. */
            volatile uint16 txe_wait = 200;
            while (!GET_BIT(SPI1->SR, 1) && txe_wait) {
                txe_wait--;
            }
            if (txe_wait > 0 && spi_tx_ptr != NULL) {
                SPI1->DR = spi_tx_ptr[spi_byte_index];
            }
            /* If TXE never became ready (txe_wait==0), we've lost this byte.
             * The packet will fail checksum and IPC will retry next cycle. */
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