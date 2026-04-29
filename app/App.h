//
// Created by Khalaf on 16/04/2026.
//

#ifndef APP_H
#define APP_H

#define APP_LCD_PINS_CONFIG { \
{GPIO_PORT_D, GPIO_PIN_0}, /* Index 0: RS */ \
{GPIO_PORT_D, GPIO_PIN_1}, /* Index 1: EN */ \
{GPIO_PORT_D, GPIO_PIN_7}, /* Index 2: D4 */ \
{GPIO_PORT_D, GPIO_PIN_8}, /* Index 3: D5 */ \
{GPIO_PORT_D, GPIO_PIN_9}, /* Index 4: D6 */ \
{GPIO_PORT_D, GPIO_PIN_10}  /* Index 5: D7 */ \
}
void App_Init(void);
void App_Run(void);

#endif //APP_H
