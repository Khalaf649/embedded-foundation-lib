//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine — Auto-Cooler
// Outputs are determined by BOTH current state AND temperature input.
//

#include "App.h"

#include "../Button/Button.h"
#include "../RCC/RCC.h"
#include "../USART/USART.h"


void App_Init(void)
{
    Rcc_Init();
    Usart1_Init();
    Button_Init();

}

void App_Run(void)
{

}