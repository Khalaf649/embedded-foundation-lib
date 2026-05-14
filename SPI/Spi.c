/*
 * Spi.c — SPI Driver with improved Slave reliability
 */

#include "Spi.h"
#include "../GPIO/GPIO.h"
#include "../Nvic/Nvic.h"
#include "../RCC/RCC.h"
#include "../include/BIT_MATH.h"
#include "stm32f4xx.h"
#include <stddef.h>

/* --- SPI Bit Definitions (STM32F4) --- */
#define SPI_CR1_CPHA (1U << 0)
#define SPI_CR1_CPOL (1U << 1)
#define SPI_CR1_MSTR (1U << 2)
#define SPI_CR1_BR_0 (1U << 3)
#define SPI_CR1_BR_1 (1U << 4)
#define SPI_CR1_BR_2 (1U << 5)
#define SPI_CR1_SPE (1U << 6)
#define SPI_CR1_LSBFIRST (1U << 7)
#define SPI_CR1_SSI (1U << 8)
#define SPI_CR1_SSM (1U << 9)
#define SPI_CR1_RXONLY (1U << 10)
#define SPI_CR1_DFF (1U << 11)

#define SPI_CR2_RXNEIE (1U << 6)
#define SPI_CR2_TXEIE (1U << 7)

#define SPI_SR_RXNE (1U << 0)
#define SPI_SR_TXE (1U << 1)
#define SPI_SR_MODF (1U << 5)
#define SPI_SR_OVR (1U << 6)
#define SPI_SR_BSY (1U << 7)

static volatile uint8 *spi_tx_ptr = NULL;
static volatile uint8 *spi_rx_ptr = NULL;
static volatile uint16 spi_length = 0;
static volatile uint16 spi_byte_index = 0;
static volatile SPI_Status_t spi_state = SPI_STATUS_IDLE;

static GPIO_Pin_Location_t sck_pin = {GPIO_PORT_A, GPIO_PIN_5};
static GPIO_Pin_Location_t miso_pin = {GPIO_PORT_A, GPIO_PIN_6};
static GPIO_Pin_Location_t mosi_pin = {GPIO_PORT_A, GPIO_PIN_7};
static GPIO_Pin_Location_t nss_pin = {GPIO_PORT_A, GPIO_PIN_4};

void Spi1_Init(void) {
  /* Enable SPI1 clock (APB2 Bit 12) */
  RCC_EnablePeripheral(108);

  GPIO_PinConfig_t spi_pins_cfg = {.mode = GPIO_MODE_AF,
                                   .pull = GPIO_PULL_NONE,
                                   .speed = GPIO_SPEED_HIGH,
                                   .otype = GPIO_OTYPE_PP};

  GPIO_InitPin(&sck_pin, &spi_pins_cfg);
  GPIO_InitPin(&miso_pin, &spi_pins_cfg);
  GPIO_InitPin(&mosi_pin, &spi_pins_cfg);

  GPIO_SetAlternateFunction(&sck_pin, GPIO_AF_5);
  GPIO_SetAlternateFunction(&miso_pin, GPIO_AF_5);
  GPIO_SetAlternateFunction(&mosi_pin, GPIO_AF_5);

#ifdef BUILD_AS_MASTER
  GPIO_PinConfig_t nss_master_cfg = {.mode = GPIO_MODE_OUTPUT,
                                     .pull = GPIO_PULL_NONE,
                                     .speed = GPIO_SPEED_HIGH,
                                     .otype = GPIO_OTYPE_PP};
  GPIO_InitPin(&nss_pin, &nss_master_cfg);
  GPIO_WritePin(&nss_pin, GPIO_HIGH);

  /* Master, Mode 0, /256 baud (max reliability) */
  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM | SPI_CR1_BR_0 |
              SPI_CR1_BR_1 | SPI_CR1_BR_2;
#else
  /* Slave: AF5 on NSS, No SSM (Hardware mode) */
  spi_pins_cfg.pull = GPIO_PULL_UP;
  GPIO_InitPin(&nss_pin, &spi_pins_cfg);
  GPIO_SetAlternateFunction(&nss_pin, GPIO_AF_5);
  SPI1->CR1 = 0; /* Slave, Mode 0, Hardware NSS */
#endif

  SET_BIT(SPI1->CR2, 6); /* RXNEIE */
  SET_BIT(SPI1->CR1, 6); /* SPE */
  Nvic_EnableInterrupt(SPI1_IRQn);
}

#ifdef BUILD_AS_MASTER
void Spi1_Master_Start_Exchange(volatile uint8 *tx_buf, volatile uint8 *rx_buf,
                                uint16 len) {
  if (spi_state == SPI_STATUS_BUSY)
    return;
  ENTER_CRITICAL();
  spi_tx_ptr = tx_buf;
  spi_rx_ptr = rx_buf;
  spi_length = len;
  spi_byte_index = 0;
  spi_state = SPI_STATUS_BUSY;

  /* Clear any pending RX data or errors before starting */
  (void)SPI1->DR;
  (void)SPI1->SR;

  GPIO_WritePin(&nss_pin, GPIO_LOW);
  EXIT_CRITICAL();
  SPI1->DR = spi_tx_ptr[0];
}
#else
void Spi1_Slave_Stage_Data(volatile uint8 *tx_buf, volatile uint8 *rx_buf,
                           uint16 len) {
  ENTER_CRITICAL();
  spi_tx_ptr = tx_buf;
  spi_rx_ptr = rx_buf;
  spi_length = len;
  spi_byte_index = 0;
  spi_state = SPI_STATUS_BUSY;

  /* Clear any pending RX data or errors to ensure sync */
  (void)SPI1->DR;
  (void)SPI1->SR;

  SPI1->DR = spi_tx_ptr[0]; /* Pre-load first byte */
  EXIT_CRITICAL();
}
#endif

SPI_Status_t Spi1_GetState(void) { return spi_state; }

void SPI1_IRQHandler(void) {
  uint32 sr = SPI1->SR;

  /* Handle RXNE */
  if (sr & SPI_SR_RXNE) {
    uint8 data = (uint8)SPI1->DR;
    if (spi_rx_ptr && spi_byte_index < spi_length) {
      spi_rx_ptr[spi_byte_index] = data;
    }
    spi_byte_index++;

    if (spi_byte_index < spi_length) {
      SPI1->DR = spi_tx_ptr[spi_byte_index];
    } else {
      spi_state = SPI_STATUS_IDLE;
#ifdef BUILD_AS_MASTER
      GPIO_WritePin(&nss_pin, GPIO_HIGH);
#endif
    }
  }

  /* Clear Errors (OVR, MODF) */
  if (sr & (SPI_SR_OVR | SPI_SR_MODF)) {
    (void)SPI1->DR; /* Clear OVR by reading DR then SR */
    (void)SPI1->SR;
  }
}
