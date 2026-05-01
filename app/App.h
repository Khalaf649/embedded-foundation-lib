//
// Created by Khalaf on 16/04/2026.
//

#ifndef APP_H
#define APP_H

#include "../include/STD_TYPES.h"

/* ── LCD Pin Mapping ──────────────────────────────── */
#define APP_LCD_PINS_CONFIG {                          \
{GPIO_PORT_B, GPIO_PIN_0},  /* Index 0: RS */      \
{GPIO_PORT_B, GPIO_PIN_10}, /* Index 1: EN */      \
{GPIO_PORT_B, GPIO_PIN_5},  /* Index 2: D4 */      \
{GPIO_PORT_B, GPIO_PIN_4},  /* Index 3: D5 */      \
{GPIO_PORT_B, GPIO_PIN_3},  /* Index 4: D6 */      \
{GPIO_PORT_B, GPIO_PIN_2}   /* Index 5: D7 */      \
}


/* ── LM35 Sensor ──────────────────────────────────── */
#define LM_PORT          GPIO_PORT_A
#define LM_PIN           GPIO_PIN_0
#define LM_CHANNEL       ADC_CHANNEL_0
#define LM_PRESCALER     ADC_PRESCALER_DIV2
#define LM_RESOLUTION    ADC_RES_12BIT

/* ── Alarm LED ────────────────────────────────────── */
#define ALARM_LED_PORT   GPIO_PORT_A
#define ALARM_LED_PIN    GPIO_PIN_5

/* ── DC Fan (TIM4 CH4 → PD15) ─────────────────────── */
#define FAN_PORT         GPIO_PORT_D
#define FAN_PIN          GPIO_PIN_15
#define FAN_TIMER        TIM_INSTANCE_4
#define FAN_CHANNEL      TIM_CHANNEL_4

/* ── Temperature Thresholds ───────────────────────── */
#define TEMP_IDLE_MAX        25.0f   /* Below this  → Fan OFF          */
#define TEMP_LOW_MAX         30.0f   /* [25,30)     → 33%              */
#define TEMP_MID_MAX         35.0f   /* [30,35)     → 66%              */
#define TEMP_HIGH_MIN        35.0f   /* ≥ 35        → 100%             */
#define TEMP_OVERHEAT_MIN    40.0f   /* ≥ 40        → OVERHEAT state   */

/* ── Duty Cycles ──────────────────────────────────── */
#define DUTY_OFF         0u
#define DUTY_LOW         33u
#define DUTY_MID         66u
#define DUTY_FULL        100u

/* ── Public API ───────────────────────────────────── */
void App_Init(void);
void App_Run(void);

#endif /* APP_H */