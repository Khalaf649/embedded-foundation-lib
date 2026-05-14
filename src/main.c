//
// main.c
// Entry point — compiles to Master or Slave based on BUILD_AS_MASTER.
//
// Build Master : gcc ... -DBUILD_AS_MASTER
// Build Slave  : gcc ... (no flag)
//

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
