//
// Created by Khalaf on 15/04/2026.
//
//
// Created by Khalaf on 15/04/2026.
//

#include "System.h"
#include "RCC.h"

/* Global Component Handles (Allocated in memory here) */
Keypad_Config_t   myKeypad;
SevenSeg_Config_t myDisplay;
LedBar_Config_t   myFeedback;
Buzzer_Config_t   myBuzzer;


void System_InitAll(void) {
    Rcc_Init();

     /* 1. Prepare the Software Configurations */
      Keypad_PrepareConfig(&myKeypad, KP_Rows, KP_Cols);
    SevenSeg_PrepareConfig(&myDisplay, SS_Pins, SEVENSEG_COMMON_CATHODE);
      LedBar_PrepareConfig(&myFeedback, Progress_Pins, Success_Pin, Alarm_Pin);
      Buzzer_PrepareConfig(&myBuzzer, Buzzer_Pin);

    /* 1. Initialize the Physical Hardware via HAL */
      Keypad_Init(&myKeypad);
    SevenSeg_Init(&myDisplay);   /* Current Focus: 7-Segment Debugging */
      LedBar_Init(&myFeedback);
       Buzzer_Init(&myBuzzer);     // <-- Initialize Buzzer Hardware
}