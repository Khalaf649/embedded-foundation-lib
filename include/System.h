//
// Created by Khalaf on 15/04/2026.
//

#ifndef SYSTEM_H
#define SYSTEM_H
#include "Keypad.h"
#include "SevenSeg.h"
#include "LedBar.h"
#include "Buzzer.h"

/* Expose the global handles so your state machine can use them later */
extern Keypad_Config_t   myKeypad;
extern SevenSeg_Config_t myDisplay;
extern LedBar_Config_t   myFeedback;
extern Buzzer_Config_t   myBuzzer;
/* Keypad (PA0-PA3 = Rows, PB0-PB3 = Columns) */
static GPIO_Pin_Location_t KP_Rows[4] = {
    {GPIO_PORT_A, GPIO_PIN_0}, {GPIO_PORT_A, GPIO_PIN_1},
    {GPIO_PORT_A, GPIO_PIN_2}, {GPIO_PORT_A, GPIO_PIN_3}
};

static GPIO_Pin_Location_t KP_Cols[4] = {
    {GPIO_PORT_A, GPIO_PIN_4}, {GPIO_PORT_A, GPIO_PIN_5},
    {GPIO_PORT_A, GPIO_PIN_6}, {GPIO_PORT_A, GPIO_PIN_7}
};

/* 7-Segment (PD0-PD6) */
static GPIO_Pin_Location_t SS_Pins[SEVENSEG_PIN_COUNT] = {
    {GPIO_PORT_C, GPIO_PIN_0}, {GPIO_PORT_C, GPIO_PIN_1},
    {GPIO_PORT_C, GPIO_PIN_2}, {GPIO_PORT_C, GPIO_PIN_3},
    {GPIO_PORT_C, GPIO_PIN_4}, {GPIO_PORT_C, GPIO_PIN_5},
    {GPIO_PORT_C, GPIO_PIN_6}
};

/* LedBar & Status LEDs (PD8-PD15 and PE0-PE1) */
/* Updated to match your Proteus schematic pins */
static GPIO_Pin_Location_t Progress_Pins[4] = {
    {GPIO_PORT_D, GPIO_PIN_0},  {GPIO_PORT_D, GPIO_PIN_1},
    {GPIO_PORT_D, GPIO_PIN_2}, {GPIO_PORT_D, GPIO_PIN_3}
};
static GPIO_Pin_Location_t Success_Pin = {GPIO_PORT_D, GPIO_PIN_4};
static GPIO_Pin_Location_t Alarm_Pin   = {GPIO_PORT_D, GPIO_PIN_5};
static GPIO_Pin_Location_t Buzzer_Pin  = {GPIO_PORT_D, GPIO_PIN_6};
/* Master Initialization API */
void System_InitAll(void);

#endif //SYSTEM_H
