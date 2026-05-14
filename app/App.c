

#include "../App/App.h"

int main(void) {
#ifdef BUILD_AS_MASTER
    App_Init();
    App_Run();          /* Never returns */
#else
    App_Slave_Init();
    App_Slave_Run();    /* Never returns */
#endif

    return 0;           /* Unreachable — satisfies compiler */
}
