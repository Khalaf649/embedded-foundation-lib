//
// Created by Khalaf on 13/05/2026.
//

#ifndef STM32_TEMPLATE_BUTTON_H
#define STM32_TEMPLATE_BUTTON_H
#include "../include/Std_Types.h"
#include "../Usart/Usart.h"

// ---------------------------------------------------------
// System State Arrays (Visible to the FSM in main.c)
// Indices 1 to 4 correspond to Floor 1 to 4 (Index 0 is unused).
// ---------------------------------------------------------

// Common for both Master and Slave
extern volatile boolean g_CabinRequests[5];
extern volatile uint8 g_Current_Floor_Sensor;
extern volatile boolean g_Emergency_Flag;

// Master Only
extern volatile boolean g_HallwayUpRequests[5];
extern volatile boolean g_HallwayDownRequests[5];

// ---------------------------------------------------------
// Initialization Function
// ---------------------------------------------------------
void Button_Init(void);
#endif //STM32_TEMPLATE_BUTTON_H
