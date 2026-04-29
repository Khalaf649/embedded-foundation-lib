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

void print_on_screen(float Temperature) {
    char tempStr[16]; // Enough for "-XX.XX\0"
    floatToStr(Temperature, tempStr);
    Usart1_TransmitString("Current Temperature: ");
    Usart1_TransmitString(tempStr);
    Usart1_TransmitString(" °C\r\n");
    Adc_StartConversion();

}
void App_Init(void) {
    System_InitAll();
    Usart1_Init();
    Lm35_GetTemperatureAsync(print_on_screen);


}
void App_Run(void) {




}





