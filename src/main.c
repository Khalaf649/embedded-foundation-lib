
#include "../app/App.h"
#include "../Usart/Usart.h"

int main(void)
{
    /* 1. Initialize everything (keypad, 7seg, LEDs, clocks, etc.) */
    App_Init();
    Usart1_TransmitString("Elevator System Started... Waiting for button press\r\n");
    while (1) {
        // App_Run();
    }

}