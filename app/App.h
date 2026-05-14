/*
 * App.h — Application entry points
 *
 * Compile-time selection:
 *  BUILD_AS_MASTER → App_Init() / App_Run() for Board A (Master)
 *  Otherwise       → App_Slave_Init() / App_Slave_Run() for Board B (Slave)
 *
 * Assumptions:
 *  - Called from main() after SystemInit/clock setup
 *  - App_Init/App_Slave_Init called exactly once
 *  - App_Run/App_Slave_Run enters infinite scheduler super-loop
 */

#ifndef APP_H
#define APP_H

/* ── Master (Board A) ──────────────────────────────────── */
#ifdef BUILD_AS_MASTER
void App_Init(void);
void App_Run(void);

/* ── Slave (Board B) ───────────────────────────────────── */
#else
void App_Slave_Init(void);
void App_Slave_Run(void);
#endif

#endif /* APP_H */
