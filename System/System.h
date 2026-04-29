//
// Created by Khalaf on 15/04/2026.
//

#ifndef SYSTEM_H
#define SYSTEM_H
#include "../Lcd/Lcd.h"
#include "../Adc/Adc.h"



extern Lcd_Config_t app_lcd;
extern Adc_Config_t app_adc;

void System_InitAll(void);

#endif //SYSTEM_H
