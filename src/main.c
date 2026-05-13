#include "STD_TYPES.h"
#include "../RCC/RCC.h"
#include "../GPIO/GPIO.h"
#include "../Usart/Usart.h"
#include "../Timer/Timer.h"     // لازم نعمل Include لدرايفر التايمر
#include "../Scheduler/Scheduler.h"

/* =========================================================
 * إعدادات الـ Hardware (اللمبة على B0)
 * ========================================================= */
static GPIO_Pin_Location_t lamp_pin = {GPIO_PORT_B, GPIO_PIN_0};

static GPIO_PinConfig_t lamp_cfg = {
    .mode  = GPIO_MODE_OUTPUT,
    .pull  = GPIO_PULL_NONE,
    .speed = GPIO_SPEED_LOW,
    .otype = GPIO_OTYPE_PP
};

/* =========================================================
 * دوال المهام (Tasks)
 * ========================================================= */
void Task_BlinkLamp(void) {
    GPIO_TogglePin(&lamp_pin);
    Usart1_TransmitString("[Task 1] Lamp Toggled on B0!\r\n");
}

void Task_SystemMonitor(void) {
    Usart1_TransmitString("[Task 2] Scheduler is running perfectly...\r\n");
}

/* =========================================================
 * الـ MAIN
 * ========================================================= */
int main(void) {


    /* 2. تهيئة الموديولات الأساسية */
    Usart1_Init();
    GPIO_InitPin(&lamp_pin, &lamp_cfg);
    GPIO_WritePin(&lamp_pin, GPIO_LOW);

    /* 3. تهيئة التايمر 4 (التعديل المهم جداً هنا!) */
    // إعداد التايمر ليحسب كل 1 ملي ثانية (بافتراض الكلوك 16MHz)

    /* 4. تهيئة الـ Scheduler (اللي هيشغل التايمر 4 بعد ما اتعمله Init) */
    Scheduler_Init();

    /* 5. تسجيل المهام (Register Tasks) */
    Scheduler_RegisterTask(Task_BlinkLamp, 1000);
    Scheduler_RegisterTask(Task_SystemMonitor, 2000);

    Usart1_TransmitString("\r\n=================================\r\n");
    Usart1_TransmitString("System Initialized.\r\n");
    Usart1_TransmitString("Starting Scheduler Test...\r\n");
    Usart1_TransmitString("=================================\r\n");

    /* 6. الـ Super Loop */
    while (1) {
        Scheduler_Run();
    }

    return 0;
}