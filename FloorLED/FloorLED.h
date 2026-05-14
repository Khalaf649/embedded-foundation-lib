/*
 * FloorLED.h — Floor indicator LEDs (PB4..PB7)
 *
 * PB4 = Floor 1,  PB5 = Floor 2,  PB6 = Floor 3,  PB7 = Floor 4
 *
 * Only the LED corresponding to the current floor is lit;
 * all others are turned off.
 */

#ifndef FLOOR_LED_H
#define FLOOR_LED_H

#include "../include/STD_TYPES.h"
#include "../ElevatorFSM/ElevatorFSM.h"

/* One-time GPIO initialisation for PB4..PB7 as push-pull outputs. */
void FloorLED_Init(void);

/* Light only the LED for the given floor (1..4). Turn off all if FLOOR_NONE. */
void FloorLED_Update(Floor_t floor);

#endif /* FLOOR_LED_H */
