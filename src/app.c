//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine implementation for Secure Keypad System
// Structure: Event-driven model with clear state transitions and outputs
//

#include "app.h"
#include "System.h"
#include "Keypad.h"
#include "SevenSeg.h"
#include "LedBar.h"
#include "exti.h"
#include "GPIO.h" // Needed to manually toggle the doorbell pin




static SystemState_t currentState = STATE_LOCKED;
static const uint8 correctPassword[APP_PASSWORD_LENGTH] = APP_DEFAULT_PASSWORD;

static uint8 inputIndex = 0;     // Tracks progress of the sequence
static uint8 failedAttempts = 0; // Tracks failed unlock attempts

static volatile boolean flag_EmergencyReset = FALSE;
static volatile boolean flag_Doorbell = FALSE;

/* --- External Hardware Handles (Defined in System.c) --- */
extern Keypad_Config_t   myKeypad;
extern SevenSeg_Config_t myDisplay;
extern LedBar_Config_t   myFeedback;

// Emergency Reset: Forces the system out of an error state [cite: 7]
void App_EmergencyResetCallback(void) {
    flag_EmergencyReset = TRUE;
}

// Door Bell: Triggers a momentary action without disrupting the lock [cite: 8]
void App_DoorbellCallback(void) {
    flag_Doorbell = TRUE;
}

void App_Init(void) {
    // 1. Initialize all physical hardware via HAL drivers
    System_InitAll();
    //
    // // 2. Register EXTI callbacks and enable lines
    // // Using EXTI_EDGE_FALLING assuming your buttons pull to GND when pressed.
    Exti_Init(EXTI_LINE_DOORBELL, EXTI_PORT_DOORBELL, EXTI_EDGE_FALLING, App_DoorbellCallback);
    Exti_Enable(EXTI_LINE_DOORBELL);

    Exti_Init(EXTI_LINE_EMERGENCY, EXTI_PORT_EMERGENCY, EXTI_EDGE_FALLING, App_EmergencyResetCallback);
    Exti_Enable(EXTI_LINE_EMERGENCY);

    // // 3. Set Initial System State [cite: 19]
    currentState = STATE_LOCKED;
    inputIndex = 0;
    failedAttempts = 0;

    // // 4. Reset UI to match initial state
    SevenSeg_WriteDigit(&myDisplay, failedAttempts);     // Display 0 failures
     LedBar_SetStatus(&myFeedback, LEDBAR_STATUS_IDLE);   // Turn off Success/Alarm LEDs
}
void App_Run(void) {

}