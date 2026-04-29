//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine implementation for Secure Keypad System
// Structure: Event-driven model with clear state transitions and outputs
//

#include "app.h"
#include "../System/System.h"
#include "../Usart/Usart.h"
#include "../Utils/Utils.h"


void App_Init(void) {
    System_InitAll();
    // 3. Print a startup message
    Usart1_TransmitString("=== Auto-Cooler Debug System ===\r\n");
    Usart1_TransmitString("System Initialized Successfully!\r\n");

}
void App_Run(void) {
    for (uint8 i = 0; i < 100; i++) {
        char tempStr[8];
        floatToStr(i, tempStr);
        Usart1_TransmitString(tempStr);

    }



}





