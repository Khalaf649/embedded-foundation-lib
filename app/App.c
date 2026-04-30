//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine — Auto-Cooler
// Outputs are determined by BOTH current state AND temperature input.
//

#include "App.h"
#include "../System/System.h"
#include "../Fan/Fan.h"
#include "../Lcd/Lcd.h"
#include "../Led/Led.h"
#include "../lm35/lm35.h"
#include "../Usart/Usart.h"
#include "../Utils/Utils.h"

/* ═══════════════════════════════════════════════════════
 *  STATE MACHINE TYPE DEFINITIONS
 * ═══════════════════════════════════════════════════════ */

typedef enum {
    STATE_IDLE     = 0,
    STATE_COOLING  = 1,
    STATE_OVERHEAT = 2
} App_State_t;

/* A state handler takes the current temperature and returns the next state */
typedef App_State_t (*StateHandler_t)(float temperature);

/* ═══════════════════════════════════════════════════════
 *  MODULE-LEVEL VARIABLES
 * ═══════════════════════════════════════════════════════ */

static App_State_t      s_current_state = STATE_IDLE;
static volatile float   s_temperature   = 0.0f;
static volatile uint8   s_new_reading   = 0u;

/* ═══════════════════════════════════════════════════════
 *  PRIVATE HELPERS
 * ═══════════════════════════════════════════════════════ */

/**
 * @brief Maps a temperature value to the correct PWM duty cycle.
 *        Pure function — no side effects.
 */
static uint8 Temp_ToDutyCycle(float temp)
{
    if      (temp < TEMP_IDLE_MAX) return DUTY_OFF;
    else if (temp < TEMP_LOW_MAX)  return DUTY_LOW;
    else if (temp < TEMP_MID_MAX)  return DUTY_MID;
    else                           return DUTY_FULL;
}

/**
 * @brief Converts a uint8 to a decimal string.
 */
static void U8_ToString(uint8 num, char *buf)
{
    if (num == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[4];
    uint8 i = 0, j = 0;
    while (num > 0) { tmp[i++] = (char)((num % 10) + '0'); num /= 10; }
    while (i > 0)   { buf[j++] = tmp[--i]; }
    buf[j] = '\0';
}

/**
 * @brief Refreshes both LCD lines.
 *        Line 1 → "Temp: XX.X C"
 *        Line 2 → "Fan: XX%"  OR  "!! OVERHEAT !!"
 */
static void LCD_Refresh(float temp, uint8 duty, uint8 is_overheat)
{
    char buf[16];

    /* Line 1: Temperature */
    Lcd_SetCursor(&app_lcd, 0, 0);
    Lcd_SendString(&app_lcd, "Temp: ");
    floatToStr(temp, buf);
    Lcd_SendString(&app_lcd, buf);
    Lcd_SendString(&app_lcd, " C      ");   /* trailing spaces clear stale chars */

    /* Line 2: Fan speed OR overheat warning */
    Lcd_SetCursor(&app_lcd, 1, 0);
    if (is_overheat) {
        Lcd_SendString(&app_lcd, "!! OVERHEAT !!  ");
    } else {
        Lcd_SendString(&app_lcd, "Fan: ");
        U8_ToString(duty, buf);
        Lcd_SendString(&app_lcd, buf);
        Lcd_SendString(&app_lcd, "%           ");
    }
}

/* ═══════════════════════════════════════════════════════
 *  ADC / LM35 INTERRUPT CALLBACK
 *  Called from interrupt context — keep it minimal.
 * ═══════════════════════════════════════════════════════ */

static void ADC_ReadingCallback(float temperature)
{
    s_temperature  = temperature;
    s_new_reading  = 1u;
    /* Re-arm for the next conversion */
    Lm35_GetTemperatureAsync(ADC_ReadingCallback);
}

/* ═══════════════════════════════════════════════════════
 *  STATE HANDLERS  (Mealy outputs + transitions)
 * ═══════════════════════════════════════════════════════ */

/**
 * STATE: IDLE
 *  Fan is OFF. ADC is running. Waiting for T ≥ 25 °C.
 */
static App_State_t State_Idle(float temp)
{
    if (temp >= TEMP_IDLE_MAX) {
        /* ── Output ─────────────────────────────── */
        uint8 duty = Temp_ToDutyCycle(temp);
        Fan_SetSpeed(&cooling_fan_cfg, duty);
        LCD_Refresh(temp, duty, 0u);
        /* ── Transition ─────────────────────────── */
        return STATE_COOLING;
    }

    /* ── Output (stay) ──────────────────────────── */
    Fan_SetSpeed(&cooling_fan_cfg, DUTY_OFF);
    LCD_Refresh(temp, DUTY_OFF, 0u);
    return STATE_IDLE;
}

/**
 * STATE: COOLING
 *  Fan is running. Speed is continuously adjusted.
 */
static App_State_t State_Cooling(float temp)
{
    if (temp >= TEMP_OVERHEAT_MIN) {
        /* ── Output ─────────────────────────────── */
        Fan_SetSpeed(&cooling_fan_cfg, DUTY_FULL);
        Led_TurnOn(&alarm_led_cfg);
        LCD_Refresh(temp, DUTY_FULL, 1u);
        /* ── Transition ─────────────────────────── */
        return STATE_OVERHEAT;
    }

    if (temp < TEMP_IDLE_MAX) {
        /* ── Output ─────────────────────────────── */
        Fan_SetSpeed(&cooling_fan_cfg, DUTY_OFF);
        LCD_Refresh(temp, DUTY_OFF, 0u);
        /* ── Transition ─────────────────────────── */
        return STATE_IDLE;
    }

    /* ── Output (stay, recalculate speed) ───────── */
    uint8 duty = Temp_ToDutyCycle(temp);
    Fan_SetSpeed(&cooling_fan_cfg, duty);
    LCD_Refresh(temp, duty, 0u);
    return STATE_COOLING;
}

/**
 * STATE: OVERHEAT
 *  Fan locked at 100%. Alarm LED ON. Waiting for T to drop below 40 °C.
 */
static App_State_t State_Overheat(float temp)
{
    if (temp < TEMP_OVERHEAT_MIN) {
        /* ── Output ─────────────────────────────── */
        Led_TurnOff(&alarm_led_cfg);
        uint8 duty = Temp_ToDutyCycle(temp);
        Fan_SetSpeed(&cooling_fan_cfg, duty);
        LCD_Refresh(temp, duty, 0u);
        /* ── Transition ─────────────────────────── */
        return STATE_COOLING;
    }

    /* ── Output (stay, maintain maximum cooling) ── */
    Fan_SetSpeed(&cooling_fan_cfg, DUTY_FULL);
    LCD_Refresh(temp, DUTY_FULL, 1u);
    return STATE_OVERHEAT;
}

/* ═══════════════════════════════════════════════════════
 *  STATE DISPATCH TABLE
 *  Index = App_State_t value — O(1) dispatch, no chains.
 * ═══════════════════════════════════════════════════════ */

static const StateHandler_t STATE_TABLE[3] = {
    State_Idle,       /* STATE_IDLE     = 0 */
    State_Cooling,    /* STATE_COOLING  = 1 */
    State_Overheat    /* STATE_OVERHEAT = 2 */
};

/* ═══════════════════════════════════════════════════════
 *  PUBLIC API
 * ═══════════════════════════════════════════════════════ */

void App_Init(void)
{
    System_InitAll();
    Usart1_Init();

    /* Kick off the first ADC conversion.
       The callback self-re-arms on every reading. */
    Lm35_GetTemperatureAsync(ADC_ReadingCallback);
}

void App_Run(void)
{
    /* Only act when the ISR has deposited a fresh reading */
    if (s_new_reading == 0u) { return; }

    /* Take a local snapshot and clear the flag atomically */
    s_new_reading    = 0u;
    float local_temp = s_temperature;

    /* Dispatch to the current state handler.
       The handler computes outputs AND returns the next state. */
    s_current_state = STATE_TABLE[s_current_state](local_temp);
}