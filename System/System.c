//
// Created by Khalaf on 15/04/2026.
//
//
// Created by Khalaf on 15/04/2026.
//

#include "System.h"

#include "../Utils/Utils.h"
#include "../RCC/RCC.h"
#include "../Usart/Usart.h"

/* Global Component Handles (Allocated in memory here) */



void System_InitAll(void) {
    Rcc_Init();
    Usart1_Init();




}