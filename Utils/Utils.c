//
// Created by Khalaf on 16/04/2026.
//

#include "Utils.h"
/**
 * Utils.c
 */

#include "Utils.h"

// SysTick Register Addresses
#define SYSTICK_CTRL  (*((volatile uint32*)0xE000E010))
#define SYSTICK_LOAD  (*((volatile uint32*)0xE000E014))
#define SYSTICK_VAL   (*((volatile uint32*)0xE000E018))

void SysTick_Init(void) {
    // Disable SysTick during setup
    SYSTICK_CTRL = 0;

    // Set reload register for 1ms interrupts (Assuming 16 MHz Clock)
    // 16,000,000 / 1000 = 16,000 ticks per millisecond
    SYSTICK_LOAD = 16000 - 1;

    // Clear current value register
    SYSTICK_VAL = 0;

    // Enable SysTick, use processor clock (Bit 0 = ENABLE, Bit 2 = CLKSOURCE)
    SYSTICK_CTRL = 0x05;
}

void delay_ms(uint32 ms) {
    for (uint32 i = 0; i < ms; i++) {
        // Clear the current value
        SYSTICK_VAL = 0;

        // Wait until the COUNTFLAG (Bit 16) is set to 1
        // This bit turns to 1 every time the timer hits 0 (which takes exactly 1ms)
        while ((SYSTICK_CTRL & (1 << 16)) == 0) {
            // Do nothing, just wait
        }
    }
}
void floatToStr(float val, char* data)
{
    if (val < 0)
    {
        *data = '-';
        data++;
        val *= -1;
    }

    int intVal = (int)(val * 100);
    data[6] = (intVal % 10) + '0';
    intVal /= 10;
    data[5] = (intVal % 10) + '0';
    intVal /= 10;

    data[4] = '.';

    data[3] = (intVal % 10) + '0';
    intVal /= 10;
    data[2] = (intVal % 10) + '0';
    intVal /= 10;
    data[1] = (intVal % 10) + '0';
    intVal /= 10;
    data[0] = (intVal % 10) + '0';

    data[7] = '\r';
    data[8] = '\n';
    data[9] = '\0';
}