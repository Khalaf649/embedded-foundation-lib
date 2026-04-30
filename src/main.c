#include "../GPIO/GPIO.h"
#include "../Timer/Timer.h"
#include "../app/App.h"

int main(void) {
    /* 1. System Clock Initialization */
    App_Init(); // Must enable Clock for GPIOA and TIM2

    /* 2. GPIO Pin Location Definition */
    GPIO_Pin_Location_t pa0_pin = {GPIO_PORT_A, GPIO_PIN_0};

    /* 3. GPIO Configuration for Timer (Alternate Function) */
    GPIO_PinConfig_t pa0_config;
    GPIO_PrepareConfig(&pa0_config,
                       GPIO_MODE_AF,      /* Use Alternate Function */
                       GPIO_PULL_NONE,
                       GPIO_SPEED_HIGH,
                       GPIO_OTYPE_PP);

    /* 4. Apply GPIO Hardware Settings */
    GPIO_InitPin(&pa0_pin, &pa0_config);

    /* 5. Route Pin PA0 to Timer 2 (AF 1) */
    /* This connects the internal TIM2_CH1 signal to the physical copper pin */
    GPIO_SetAlternateFunction(&pa0_pin, GPIO_AF_1);

    /* 6. Timer Timebase Setup */
    /* Prescaler: 1ms ticks | Period: 500ms
     * Result: The pin will change state every 500ms (1 second for full cycle)
     */
    Timer_Init(TIM_INSTANCE_2, TIM_PRESCALER_1MS_TICK, 500);

    /* 7. Channel Setup (Toggle Mode) */
    /* This tells the timer to flip the pin state on every match */
    Timer_ConfigChannel(TIM_INSTANCE_2, TIM_CHANNEL_1, 15999/3,TIM_OC_TOGGLE,1000);

    /* 8. The Loop */
    while (1) {
        /* The CPU is doing absolutely nothing.
         * If you check PA0 with an Oscilloscope or LED,
         * it will be blinking automatically.
         */
    }

    return 0;
}