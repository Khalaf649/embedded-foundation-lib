/*
 * FloorLED.c — Floor indicator LEDs on PB4..PB7
 *
 * PB4 = Floor 1,  PB5 = Floor 2,  PB6 = Floor 3,  PB7 = Floor 4
 *
 * Only the LED matching the current floor is ON; all others are OFF.
 */

#include "FloorLED.h"
#include "../GPIO/GPIO.h"
#include "../Led/Led.h"

/* --- Private: one Led_Config_t per floor --- */
static Led_Config_t s_floor_leds[NUM_FLOORS];   /* index 0 = Floor 1, … 3 = Floor 4 */

/* Pin mapping: PB4, PB5, PB6, PB7 */
static const GPIO_Pin_t s_led_pins[NUM_FLOORS] = {
    GPIO_PIN_4,   /* Floor 1 */
    GPIO_PIN_5,   /* Floor 2 */
    GPIO_PIN_6,   /* Floor 3 */
    GPIO_PIN_7    /* Floor 4 */
};

/* ================================================================
 * FloorLED_Init
 * ================================================================ */
void FloorLED_Init(void) {
    for (uint8 i = 0; i < NUM_FLOORS; i++) {
        GPIO_Pin_Location_t pin_loc = { GPIO_PORT_B, s_led_pins[i] };
        Led_PrepareConfig(&s_floor_leds[i], pin_loc);
        Led_Init(&s_floor_leds[i]);
        /* All LEDs start OFF */
    }
    /* Light Floor 1 by default (elevator starts at floor 1) */
    Led_TurnOn(&s_floor_leds[0]);
}

/* ================================================================
 * FloorLED_Update — light only the LED for 'floor', turn off all others
 * ================================================================ */
void FloorLED_Update(Floor_t floor) {
    uint8 f = (uint8)floor;

    for (uint8 i = 0; i < NUM_FLOORS; i++) {
        if (f >= 1 && f <= NUM_FLOORS && i == (f - 1U)) {
            Led_TurnOn(&s_floor_leds[i]);
        } else {
            Led_TurnOff(&s_floor_leds[i]);
        }
    }
}
