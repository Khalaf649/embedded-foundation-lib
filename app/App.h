//
// Created by Khalaf on 16/04/2026.
//

#ifndef APP_H
#define APP_H

// ── Master (Board A) ─────────────────────────────────────
#ifdef BUILD_AS_MASTER
void App_Init(void);   
void App_Run(void);

// ── Slave (Board B) ──────────────────────────────────────
#else
void App_Slave_Init(void);
void App_Slave_Run(void);
#endif

#endif // APP_H
