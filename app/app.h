//
// Created by Khalaf on 16/04/2026.
//

#ifndef APP_H
#define APP_H
#include "../exti/exti.h"

typedef enum {
    STATE_LOCKED,    // Initial State
    STATE_UNLOCKED,
    STATE_ALARM
} SystemState_t;
#define APP_PASSWORD_LENGTH       4
#define APP_MAX_FAILED_ATTEMPTS   3
#define APP_DEFAULT_PASSWORD      {'1', '8', '1', '2'}

// Changed from '#' to 'C' to match your specific keypad's ON/C button
#define APP_LOCK_COMMAND_KEY      '+'

// Doorbell EXTI Mapping
#define EXTI_LINE_DOORBELL   EXTI_LINE_13
#define EXTI_PORT_DOORBELL   EXTI_PORT_B

// Emergency Reset EXTI Mapping (Adjust line/port to match your wiring)
#define EXTI_LINE_EMERGENCY  EXTI_LINE_4
#define EXTI_PORT_EMERGENCY  EXTI_PORT_B


void App_Init(void);
void App_Run(void);

#endif //APP_H
