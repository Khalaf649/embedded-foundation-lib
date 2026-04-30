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
#include "../lm35/lm35.h"
#include "../Led/Led.h"
#include "../Fan/Fan.h"

Lcd_Config_t app_lcd;
Led_Config_t alarm_led_cfg;
Fan_Config_t cooling_fan_cfg;
// 2. Load the user's pin choices from App.h into the local array
static GPIO_Pin_Location_t Lcd_Pins[6] = APP_LCD_PINS_CONFIG;

void System_InitAll(void) {
    Rcc_Init();
    Lcd_PrepareConfig(&app_lcd,
                  Lcd_Pins[0],  // RS
                  Lcd_Pins[1],  // EN
                  Lcd_Pins[2],  // D4
                  Lcd_Pins[3],  // D5
                  Lcd_Pins[4],  // D6
                  Lcd_Pins[5]); // D7

    Lcd_Init(&app_lcd);

    Lm35_Init(LM_PORT,
              LM_PIN,
              LM_CHANNEL,
              LM_PRESCALER,
              LM_RESOLUTION);

    GPIO_Pin_Location_t alarm_pin_loc = {ALARM_LED_PORT, ALARM_LED_PIN};
    Led_PrepareConfig(&alarm_led_cfg, alarm_pin_loc);
    Led_Init(&alarm_led_cfg);

    /* --- 5. DC Fan PWM Initialization --- */

    /* A. Define the pin location (PD15 based on schematic) */
    GPIO_Pin_Location_t fan_pin_loc = {FAN_PORT, FAN_PIN};

    /* B. Prepare the GPIO configuration for Alternate Function Mode */
    GPIO_PinConfig_t fan_gpio_cfg;
    GPIO_PrepareConfig(&fan_gpio_cfg,
                       GPIO_MODE_AF,       /* Essential: Set pin to Alternate Function mode */
                       GPIO_PULL_NONE,     /* No pull-up/down needed for transistor base */
                       GPIO_SPEED_HIGH,    /* High speed for PWM switching integrity */
                       GPIO_OTYPE_PP);     /* Push-Pull output */

    /* C. Initialize the physical pin hardware */
    GPIO_InitPin(&fan_pin_loc, &fan_gpio_cfg);

    /* D. Connect the pin to the Timer (AF2 for TIM4 on PD15) */
    GPIO_SetAlternateFunction(&fan_pin_loc, GPIO_AF_2);

    /* E. Initialize the Fan Driver Logic */
    cooling_fan_cfg.timer_instance = FAN_TIMER;
    cooling_fan_cfg.channel = FAN_CHANNEL;

    Fan_Init(&cooling_fan_cfg);
    Fan_SetSpeed(&cooling_fan_cfg, 0);






}