//
// Created by Khalaf on 16/04/2026.
//
// Mealy State Machine implementation for Secure Keypad System
// Structure: Event-driven model with clear state transitions and outputs
//

#include "app.h"
#include "../System/System.h"
#include "../Keypad/Keypad.h"
#include "../SevenSeg/SevenSeg.h"
#include "../LedBar/LedBar.h"
#include "../exti/exti.h"




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
static void App_HandleLockedState(boolean isKeyPressed, uint8 key) {
    // Input Event: Any keypad press detected that is NOT the lock command
    if (isKeyPressed == TRUE && key != APP_LOCK_COMMAND_KEY) {

        // Event: Valid Sequence Input Detected
        if (key == correctPassword[inputIndex]) {
            inputIndex++;
            LedBar_SetProgress(&myFeedback, inputIndex);

            // Condition Check: Is the Sequence Complete?
            if (inputIndex >= APP_PASSWORD_LENGTH) {
                inputIndex = 0;
                failedAttempts = 0;
                SevenSeg_WriteDigit(&myDisplay, failedAttempts);
                LedBar_SetStatus(&myFeedback, LEDBAR_STATUS_SUCCESS);
                currentState = STATE_UNLOCKED;
            }
        }

        // Event: Invalid Sequence Input Detected
        else {
            inputIndex = 0;
            LedBar_SetProgress(&myFeedback, 0);

            failedAttempts++;
            SevenSeg_WriteDigit(&myDisplay, failedAttempts);

            // Condition Check: Lockout Threshold Reached?
            if (failedAttempts >= APP_MAX_FAILED_ATTEMPTS) {
                LedBar_SetStatus(&myFeedback, LEDBAR_STATUS_ALARM);
                currentState = STATE_ALARM;
            }
        }
    }
}

static void App_HandleUnlockedState(boolean isKeyPressed, uint8 key) {
    // Event: Lock Command Triggered
    if (isKeyPressed == TRUE && key == APP_LOCK_COMMAND_KEY) {
        LedBar_SetStatus(&myFeedback, LEDBAR_STATUS_IDLE);
        currentState = STATE_LOCKED;
    }
}

static void App_HandleAlarmState(void) {
    // Event: Emergency Reset Triggered via EXTI
    if (flag_EmergencyReset) {
        flag_EmergencyReset = 0; // Acknowledge and clear flag

        // Clear alarm, clear progress, reset history
        LedBar_SetStatus(&myFeedback, LEDBAR_STATUS_IDLE);

        inputIndex = 0;
        LedBar_SetProgress(&myFeedback, 0);

        failedAttempts = 0;
        SevenSeg_WriteDigit(&myDisplay, failedAttempts);

        currentState = STATE_LOCKED;
    }
}

void App_Init(void) {
    // 1. Initialize all physical hardware via HAL drivers
    System_InitAll();
    //
    // // 2. Register EXTI callbacks and enable lines
    // // Using EXTI_EDGE_FALLING assuming your buttons pull to GND when pressed.
    Exti_Init(EXTI_LINE_DOORBELL, EXTI_PORT_DOORBELL, EXTI_EDGE_BOTH, App_DoorbellCallback);
    Exti_Enable(EXTI_LINE_DOORBELL);

    Exti_Init(EXTI_LINE_EMERGENCY, EXTI_PORT_EMERGENCY, EXTI_EDGE_RISING, App_EmergencyResetCallback);
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
    // 1. Read Current Input from the Keypad
    uint8 key = 0;
    boolean isKeyPressed = Keypad_GetPressedKey(&myKeypad, &key);

    // 2. GLOBAL EVENT: Doorbell works in ALL states
    if (flag_Doorbell) {
        flag_Doorbell = 0; // Acknowledge and clear flag
        Buzzer_Toggle(&myBuzzer); // Pulse doorbell indicator
    }
    if (flag_EmergencyReset && currentState != STATE_ALARM) {
        flag_EmergencyReset = 0;
    }

    // 3. Evaluate State Machine
    switch (currentState) {
        case STATE_LOCKED:
            App_HandleLockedState(isKeyPressed, key);
        break;

        case STATE_UNLOCKED:
            App_HandleUnlockedState(isKeyPressed, key);
        break;

        case STATE_ALARM:
            App_HandleAlarmState();
        break;
    }
}