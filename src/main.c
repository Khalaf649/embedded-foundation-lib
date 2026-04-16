
#include "../app/app.h"
#include "../System/System.h"
// contains myKeypad and myDisplay

int main(void)
{
    /* 1. Initialize everything (keypad, 7seg, LEDs, clocks, etc.) */
    App_Init();
    while (1) {
        App_Run();
    }

}