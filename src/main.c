#include "STD_TYPES.h"
#include "../RCC/RCC.h"
#include "../GPIO/GPIO.h"
#include "../Usart/Usart.h"
#include "../Timer/Timer.h"
#include "../Scheduler/Scheduler.h"
#include "../IPC/Ipc.h"
#include "../SPI/Spi.h" // للتأكد من وجود تعريفات SPI_ROLE_MASTER وما شابه

/* --- دالة مساعدة لطباعة الأرقام بدون استخدام sprintf --- */
void Usart_PrintNumber(uint32 num) {
    if (num == 0) {
        Usart1_TransmitByte('0');
        return;
    }
    char buf[10];
    int i = 0;

    // استخراج الأرقام من اليمين للشمال
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    // طباعة الأرقام بالترتيب الصحيح
    while (i > 0) {
        Usart1_TransmitByte(buf[--i]);
    }
}

/* --- Tasks Definition --- */
void Task_IPCSync(void) {
    Ipc_Sync();
}

void Task_SimulateElevator(void) {
#ifdef BUILD_AS_MASTER
    if (g_tx_packet.target_floor == FLOOR_4) {
        g_tx_packet.target_floor = FLOOR_1;
    } else {
        g_tx_packet.target_floor++;
    }
    g_tx_packet.lift_state = LIFT_STATE_MOVING_UP;
    g_tx_packet.motor_speed = MOTOR_HIGH_SPEED;
#else
    if (g_rx_packet.target_floor != FLOOR_NONE && g_rx_packet.target_floor != g_tx_packet.current_floor) {
        g_tx_packet.current_floor = g_rx_packet.target_floor;
        g_tx_packet.lift_state = LIFT_STATE_IDLE;
        g_tx_packet.motor_speed = MOTOR_REST;
    }
#endif
}

void Task_Telemetry(void) {
    if (g_comm_fault == TRUE) {
        Usart1_TransmitString("\r\n[!!! FAULT !!!] IPC Communication Lost!\r\n");
        return;
    }

#ifdef BUILD_AS_MASTER
    Usart1_TransmitString("[MASTER] I Sent Target: ");
    Usart_PrintNumber(g_tx_packet.target_floor);
    Usart1_TransmitString(" | Slave is at Floor: ");
    Usart_PrintNumber(g_rx_packet.current_floor);
    Usart1_TransmitString("\r\n");
#else
    Usart1_TransmitString("[SLAVE] Master ordered Target: ");
    Usart_PrintNumber(g_rx_packet.target_floor);
    Usart1_TransmitString(" | My Floor: ");
    Usart_PrintNumber(g_tx_packet.current_floor);
    Usart1_TransmitString("\r\n");
#endif
}

int main(void) {
    /* 1. السطور الأساسية الناقصة (RCC و Timer) */


    /* 2. تهيئة الموديولات */
    Usart1_Init();
    Ipc_Init();

    /* 3. تصحيح نداء الـ SPI بالباراميترات الجديدة */
#ifdef BUILD_AS_MASTER
    Spi1_Init();
    Usart1_TransmitString("\r\n>>> IPC Test: MASTER Node Started <<<\r\n");
#else
    Spi1_Init();
    Usart1_TransmitString("\r\n>>> IPC Test: SLAVE Node Started <<<\r\n");
#endif

    /* 4. الـ Scheduler Setup */
    Scheduler_Init();
    Scheduler_RegisterTask(Task_IPCSync, 50);
    Scheduler_RegisterTask(Task_SimulateElevator, 2000);
    Scheduler_RegisterTask(Task_Telemetry, 500);

    /* 5. الـ Super Loop */
    while (1) {
        Scheduler_Run();
    }

    return 0;
}