//
// Created by Khalaf on 15/04/2026.
//

#ifndef SYSTEM_H
#define SYSTEM_H
#include "../Lcd/Lcd.h"
#include "../Adc/Adc.h"
#include "../Led/Led.h"
#include "../Fan/Fan.h"



extern Lcd_Config_t app_lcd;
extern Led_Config_t alarm_led_cfg;
extern Fan_Config_t cooling_fan_cfg;

void System_InitAll(void);

#endif //SYSTEM_H
