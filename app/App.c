//
// Created by Khalaf on 16/04/2026.
//

#include "App.h"
#include "../Button/Button.h"
#include "../RCC/RCC.h"
#include "../Usart/Usart.h"
#include "../Motor/Motor.h"
#include "../Timer/Timer.h"

static uint8 led_state = 0;

static void App_BlinkCallback(void) {
    if (led_state == 0) {
        Motor_SetSpeed(MOTOR_HIGH_SPEED);
        led_state = 1;
    } else {
        Motor_SetSpeed(MOTOR_REST);
        led_state = 0;
    }
}

void App_Init(void)
{
    Rcc_Init();
    Usart1_Init();
    Button_Init();
    Motor_Init();

    Usart1_TransmitString("Hardware Initialized...\r\n");

    // ========================================================
    // 1. تهيئة التايمر (أهم خطوة عشان الـ RCC يفتح الكهرباء)
    // Prescaler = 15, AutoReload = 999 (بافتراض الكلوك 16MHz)
    // ========================================================
    Timer_Init(TIM_INSTANCE_4, 15, 999);

    // ========================================================
    // 2. تشغيل التايمر في وضع الـ Periodic وربطه بالـ Callback
    // هيعد 1000 ملي ثانية بناءً على تظبيطة الـ Init
    // ========================================================
    Timer_StartPeriodic(TIM_INSTANCE_4, 1000, App_BlinkCallback);

    Usart1_TransmitString("Timer 4 Started: Blinking LED every 1 second\r\n");
}

void App_Run(void)
{
    // فارغة تماماً، كل حاجة شغالة بالـ Interrupts
}