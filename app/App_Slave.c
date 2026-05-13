// //
// // Created by zramd on 13/05/2026.
// //
//
// #include "App.h"
// #include "../RCC/RCC.h"
// #include "../Usart/Usart.h"
// #include "../SPI/Spi.h"
// #include "../IPC/Ipc.h"
// #include "../Dispatcher/Dispatcher.h"
//
// static volatile Ipc_Packet_t my_elevator_state;
//
// void App_Slave_Init(void) {
//     Rcc_Init();
//     Usart1_Init();
//     Spi2_Init_Slave(); // Initialize as SLAVE
//
//     // Init local slave state
//     my_elevator_state.state = ELEV_IDLE;
//     my_elevator_state.current_floor = 4; // Start on different floor for testing
//
//     // Pre-load the first transmission packet so it's ready when master clocks
//     tx_packet = my_elevator_state;
//     tx_packet.header = IPC_HEADER_BYTE;
//     tx_packet.checksum = IPC_CalculateChecksum(&tx_packet);
//
//     Usart1_TransmitString("Elevator Slave Node Initialized. Waiting for Master...\r\n");
// }
//
// void App_Slave_Run(void) {
//     while(1) {
//         if (spi_transfer_complete) {
//             ENTER_CRITICAL();
//             spi_transfer_complete = 0; // Reset flag
//             EXIT_CRITICAL();
//
//             // Verify incoming data from Master
//             if (rx_packet.header == IPC_HEADER_BYTE &&
//                 rx_packet.checksum == IPC_CalculateChecksum(&rx_packet)) {
//
//                 Usart1_TransmitString("Valid Packet Received from Master!\r\n");
//
//                 // Example: Prepare local state for the NEXT transaction
//                 tx_packet = my_elevator_state;
//                 tx_packet.header = IPC_HEADER_BYTE;
//                 tx_packet.checksum = IPC_CalculateChecksum(&tx_packet);
//                 }
//         }
//     }
// }