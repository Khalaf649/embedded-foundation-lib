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
#define LM_PORT          GPIO_PORT_A
#define LM_PIN           GPIO_PIN_0
#define LM_CHANNEL       ADC_CHANNEL_0
#define LM_PRESCALER     ADC_PRESCALER_DIV2
#define LM_RESOLUTION    ADC_RES_12BIT

void App_Init(void);
void App_Run(void);

#endif //APP_H
