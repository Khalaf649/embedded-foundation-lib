#include "../app/App.h"
#include "../Usart/Usart.h"

int main(void)
{
#ifdef BUILD_AS_MASTER
    App_Init();
    App_Run();
    Usart1_TransmitString("Elevator System Started... Waiting for button press\r\n");
#else
    App_Slave_Init();
    App_Slave_Run();
#endif

    while (1) {
    }
}