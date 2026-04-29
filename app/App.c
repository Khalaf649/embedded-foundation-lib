//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine implementation for Secure Keypad System
// Structure: Event-driven model with clear state transitions and outputs
//

#include "App.h"

#include "../lm35/lm35.h"
#include "../System/System.h"
#include "../Usart/Usart.h"
#include "../Utils/Utils.h"
#include "../ADC/Adc.h"


void App_Init(void) {
    System_InitAll();
    Usart1_Init();

}
void App_Run(void) {
    float temp = Lm35_GetTemperature();
    char tempStr[16]; // Enough for "-XX.XX\0"
    floatToStr(temp, tempStr);
    Usart1_TransmitString("Current Temperature: ");
    Usart1_TransmitString(tempStr);
    Usart1_TransmitString(" °C\r\n");



}





