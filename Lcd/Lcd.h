//
// Created by Khalaf on 29/04/2026.
//

#ifndef LCD_H
#define LCD_H
#include "Std_Types.h"
#include "../GPIO/GPIO.h"
typedef struct {
    GPIO_Pin_Location_t rs_pin;
    GPIO_Pin_Location_t en_pin;
    GPIO_Pin_Location_t d4_pin;
    GPIO_Pin_Location_t d5_pin;
    GPIO_Pin_Location_t d6_pin;
    GPIO_Pin_Location_t d7_pin;
} Lcd_Config_t;

typedef Lcd_Config_t* Lcd_Config_Handle_t;


/* --- CORE APIs --- */

// Prepares the struct with the pin locations
void Lcd_PrepareConfig(P_void config_out,
                       GPIO_Pin_Location_t rs,
                       GPIO_Pin_Location_t en,
                       GPIO_Pin_Location_t d4,
                       GPIO_Pin_Location_t d5,
                       GPIO_Pin_Location_t d6,
                       GPIO_Pin_Location_t d7);

#define LCD_DELAY_TIMER    TIM_INSTANCE_3    // Free timer dedicated to LCD delays


// Initializes the GPIO pins and runs the LCD 4-bit startup sequence
void Lcd_Init(const Lcd_Config_Handle_t lcd_h);

// Send command and data require the handle now so they know which pins to toggle
void Lcd_SendCommand(const Lcd_Config_Handle_t lcd_h, uint8 cmd);
void Lcd_SendData(const Lcd_Config_Handle_t lcd_h, uint8 data);

void Lcd_SendString(const Lcd_Config_Handle_t lcd_h, const char* str);
void Lcd_SetCursor(const Lcd_Config_Handle_t lcd_h, uint8 row, uint8 col);
void Lcd_Clear(const Lcd_Config_Handle_t lcd_h);

#endif //LCD_H
