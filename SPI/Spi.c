#include "Spi.h"
#include "../IPC/Ipc.h"
#include "stm32f4xx.h" // Required for registers and IRQ

volatile uint8 spi_transfer_complete = 0;
Ipc_Packet_t tx_packet;
Ipc_Packet_t rx_packet;

void Spi2_Init_Master(void) {
    // 1. Enable Clocks for GPIOB and SPI2
    RCC->AHB1ENR |= (1 << 1);  // GPIOBEN
    RCC->APB1ENR |= (1 << 14); // SPI2EN

    // 2. Configure PB13, PB14, PB15 as Alternate Function (AF5)
    GPIOB->MODER &= ~((3<<(13*2)) | (3<<(14*2)) | (3<<(15*2)));
    GPIOB->MODER |=  ((2<<(13*2)) | (2<<(14*2)) | (2<<(15*2)));
    GPIOB->AFR[1] &= ~((0xF<<(5*4)) | (0xF<<(6*4)) | (0xF<<(7*4)));
    GPIOB->AFR[1] |=  ((5<<(5*4)) | (5<<(6*4)) | (5<<(7*4))); // AF5 for SPI2

    // 3. Configure PB12 as Output for Software Chip Select (CS)
    GPIOB->MODER &= ~(3<<(12*2));
    GPIOB->MODER |=  (1<<(12*2));
    GPIOB->ODR |= (1<<12); // Set CS High initially

    // 4. Configure SPI2 (Master, fPCLK/8, CPOL=0, CPHA=0, Software Slave Management)
    SPI2->CR1 = (1 << 2) | (1 << 4) | (1 << 9) | (1 << 8); // MSTR, BR=001, SSM, SSI
    SPI2->CR2 = (1 << 6); // RXNEIE - Enable RX Buffer Not Empty Interrupt

    // 5. Enable SPI and NVIC
    SPI2->CR1 |= (1 << 6); // SPE - SPI Enable
    NVIC_EnableIRQ(SPI2_IRQn);
}

void Spi2_Init_Slave(void) {
    // 1. Enable Clocks for GPIOB and SPI2
    RCC->AHB1ENR |= (1 << 1);
    RCC->APB1ENR |= (1 << 14);

    // 2. Configure PB12, PB13, PB14, PB15 as Alternate Function (AF5)
    GPIOB->MODER &= ~((3<<(12*2)) | (3<<(13*2)) | (3<<(14*2)) | (3<<(15*2)));
    GPIOB->MODER |=  ((2<<(12*2)) | (2<<(13*2)) | (2<<(14*2)) | (2<<(15*2)));
    GPIOB->AFR[1] &= ~((0xF<<(4*4)) | (0xF<<(5*4)) | (0xF<<(6*4)) | (0xF<<(7*4)));
    GPIOB->AFR[1] |=  ((5<<(4*4)) | (5<<(5*4)) | (5<<(6*4)) | (5<<(7*4)));

    // 3. Configure SPI2 (Slave, Hardware NSS)
    SPI2->CR1 = 0; // Default is Slave, CPOL=0, CPHA=0
    SPI2->CR2 = (1 << 6); // RXNEIE

    // 4. Enable SPI and NVIC
    SPI2->CR1 |= (1 << 6); // SPE
    NVIC_EnableIRQ(SPI2_IRQn);
}

void Spi2_Start_Exchange(Ipc_Packet_t* local_data) {
    ENTER_CRITICAL();
    tx_packet = *local_data;
    tx_packet.header = IPC_HEADER_BYTE;
    tx_packet.checksum = IPC_CalculateChecksum(&tx_packet);
    spi_transfer_complete = 0;

    // Master pulls CS Low
    GPIOB->ODR &= ~(1<<12);
    EXIT_CRITICAL();

    // Trigger first byte transmission
    SPI2->DR = ((uint8*)&tx_packet)[0];
}

void SPI2_IRQHandler(void) {
    static uint8 byte_index = 0;
    uint8* rx_ptr = (uint8*)&rx_packet;
    uint8* tx_ptr = (uint8*)&tx_packet;

    if (SPI2->SR & (1 << 0)) { // If RXNE flag is set
        rx_ptr[byte_index] = SPI2->DR; // Read received byte
        byte_index++;

        if (byte_index < sizeof(Ipc_Packet_t)) {
            SPI2->DR = tx_ptr[byte_index]; // Send next byte
        } else {
            byte_index = 0;
            spi_transfer_complete = 1;

            // If Master, pull CS High to end transaction
            if (SPI2->CR1 & (1 << 2)) {
                GPIOB->ODR |= (1<<12);
            }
        }
    }
}