//
// Created by Khalaf on [Current Date].
//

#include "App.h"
#include "../RCC/RCC.h"
#include "../Usart/Usart.h" // الـ Header بتاعك
#include "../SPI/Spi.h"

/* --- متغيرات الاختبار (1 Byte) --- */
volatile uint8 tx_char = 0;
volatile uint8 rx_char = 0;

/* ==================================================================== */
/* MASTER LOGIC (Board A)                                               */
/* ==================================================================== */
#ifdef BUILD_AS_MASTER

void App_Init(void) {
    Rcc_Init();
    Usart1_Init(); // تهيئة الـ USART
    Spi1_Init(); // إعداد الماستر

    Usart1_TransmitString("\r\n=================================\r\n");
    Usart1_TransmitString("[MASTER] SPI Test Ready!\r\n");
    Usart1_TransmitString("Type any character in this terminal...\r\n");
    Usart1_TransmitString("=================================\r\n");
}

void App_Run(void) {
    /* 1. الانتظار حتى يقوم المستخدم بكتابة حرف على الـ Terminal
     * نستخدم دالتك بالظبط: Usart1_RecieveByte
     */
    tx_char = Usart1_RecieveByte();

    /* 2. التأكد أن الـ SPI جاهز ثم إرسال الحرف */
    if (Spi1_GetState() == SPI_STATUS_IDLE) {

        // إرسال بايت واحد فقط
        Spi1_Master_Start_Exchange(&tx_char, &rx_char, 1);

        // انتظار انتهاء الإرسال (لغرض الاختبار فقط)
        while(Spi1_GetState() == SPI_STATUS_BUSY);

        /* 3. طباعة التأكيد وما تم استلامه من السليف */
        Usart1_TransmitString("\r\n[MASTER] Sent: ");
        Usart1_TransmitByte(tx_char); // استخدام دالتك TransmitByte
        Usart1_TransmitString(" | Received from Slave: ");
        Usart1_TransmitByte(rx_char);
        Usart1_TransmitString("\r\n> ");
    }
}

/* ==================================================================== */
/* SLAVE LOGIC (Board B)                                                */
/* ==================================================================== */
#else

void App_Slave_Init(void) {
    Rcc_Init();
    Usart1_Init();
    Spi1_Init(); // إعداد السليف

    Usart1_TransmitString("\r\n=================================\r\n");
    Usart1_TransmitString("[SLAVE] SPI Test Ready!\r\n");
    Usart1_TransmitString("Waiting for data from Master...\r\n");
    Usart1_TransmitString("=================================\r\n");

    /* تجهيز الرد المبدئي للسليف */
    tx_char = '*';

    /* "The Slave Challenge": تجهيز الداتا قبل أن يبدأ الماستر */
    Spi1_Slave_Stage_Data(&tx_char, &rx_char, 1);
}

void App_Slave_Run(void) {
    static SPI_Status_t last_state = SPI_STATUS_IDLE;
    SPI_Status_t current_state = Spi1_GetState();

    /* اكتشاف اللحظة التي ينتهي فيها النقل (تغير الحالة من BUSY إلى IDLE) */
    if (last_state == SPI_STATUS_BUSY && current_state == SPI_STATUS_IDLE) {

        /* 1. طباعة الحرف الذي وصل من الماستر */
        Usart1_TransmitString("\r\n[SLAVE] Received from Master: ");
        Usart1_TransmitByte(rx_char); // استخدام دالتك TransmitByte

        /* 2. تجهيز الرد للنبضة القادمة (Echo) */
        tx_char = rx_char;

        /* 3. إعادة تحميل البيانات فوراً للدورة القادمة */
        Spi1_Slave_Stage_Data(&tx_char, &rx_char, 1);
    }

    last_state = current_state;
}

#endif