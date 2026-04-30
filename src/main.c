#include "../GPIO/GPIO.h"
#include "../Timer/Timer.h"
#include "../app/App.h"

/* We make the pin global so the callback function can see it */
GPIO_Pin_Location_t led_pin = {GPIO_PORT_A, GPIO_PIN_5};

/* ==================================================================== */
/* --- The Callback Function (Interrupt Service Routine handler) --- */
/* ==================================================================== */
void MyTimerBlinkCallback(void) {
    /* 1. Toggle the LED */
    GPIO_TogglePin(&led_pin);

    /* 2. Start the timer again for the next cycle (1000 ms) */
    Timer_DelayMsAsync(TIM_INSTANCE_2, 1000, MyTimerBlinkCallback);
}

/* ==================================================================== */
/* --- Main Application --- */
/* ==================================================================== */
int main(void) {
    /* 1. Initialize Clocks */
    App_Init();

    /* 2. Configure the Pin as a STANDARD OUTPUT */
    GPIO_PinConfig_t led_config;
    GPIO_PrepareConfig(&led_config,
                       GPIO_MODE_OUTPUT,
                       GPIO_PULL_NONE,
                       GPIO_SPEED_LOW,
                       GPIO_OTYPE_PP);

    GPIO_InitPin(&led_pin, &led_config);

    /* 3. ENABLE THE NVIC (CRITICAL)
     * The Timer generates an interrupt, but the NVIC (Nested Vectored
     * Interrupt Controller) acts as the gatekeeper. If the NVIC door
     * is closed, the CPU will never hear the interrupt.
     *
     * TIM2 is usually IRQ number 28 on STM32s.
     * Because 28 is less than 32, it lives in the ISER[0] register.
     */
    /* Un-comment the line below if you don't have an NVIC driver yet: */
    // *(volatile uint32*)(0xE000E100) |= (1UL << 28);

    /* 4. Start the Asynchronous Delay (1 Second) */
    /* This tells the timer: "Count to 1000ms, then call MyTimerBlinkCallback" */
    Timer_DelayMsAsync(TIM_INSTANCE_2, 1000, MyTimerBlinkCallback);

    /* 5. The Infinite Loop */
    while (1) {
        /* The CPU is completely free!
         * It is NOT trapped in a while loop counting time.
         * You can write code here to read sensors, do math, or sleep.
         * The LED is blinking purely in the background.
         */
    }

    return 0;
}