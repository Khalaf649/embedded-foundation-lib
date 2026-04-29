//
// Created by Khalaf on 15/04/2026.
//
//
// Created by Khalaf on 15/04/2026.
//

#include "System.h"
#include "../app/App.h"
#include "../RCC/RCC.h"
#include "../Usart/Usart.h"
#include "../Lcd/Lcd.h"

Lcd_Config_t app_lcd;

// 2. Load the user's pin choices from App.h into the local array
static GPIO_Pin_Location_t Lcd_Pins[6] = APP_LCD_PINS_CONFIG;

void System_InitAll(void) {
    Rcc_Init();
    Usart1_Init();
    Lcd_PrepareConfig(&app_lcd,
                  Lcd_Pins[0],  // RS
                  Lcd_Pins[1],  // EN
                  Lcd_Pins[2],  // D4
                  Lcd_Pins[3],  // D5
                  Lcd_Pins[4],  // D6
                  Lcd_Pins[5]); // D7

    Lcd_Init(&app_lcd);




}