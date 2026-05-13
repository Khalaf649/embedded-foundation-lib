#include "Button.h"
#include "../exti/exti.h"
#include "../RCC/RCC.h"

// ---------------------------------------------------------
//  (State Variables)
// ---------------------------------------------------------
volatile boolean g_CabinRequests[5]       = {FALSE, FALSE, FALSE, FALSE, FALSE}; 
volatile uint8   g_Current_Floor_Sensor   = 1; 
volatile boolean g_Emergency_Flag         = FALSE;

volatile boolean g_HallwayUpRequests[5]   = {FALSE, FALSE, FALSE, FALSE, FALSE}; 
volatile boolean g_HallwayDownRequests[5] = {FALSE, FALSE, FALSE, FALSE, FALSE}; 

// =========================================================
// 1. CABIN CALLBACKS (PA0 -> PA3)
// =========================================================
void Cabin_F1_CB(void) { g_CabinRequests[1] = TRUE; }
void Cabin_F2_CB(void) { g_CabinRequests[2] = TRUE; }
void Cabin_F3_CB(void) { g_CabinRequests[3] = TRUE; }
void Cabin_F4_CB(void) { g_CabinRequests[4] = TRUE; }

// =========================================================
// 2. SENSOR CALLBACKS (PB4 -> PB7)
// =========================================================
void Sensor_F1_CB(void) { g_Current_Floor_Sensor = 1; }
void Sensor_F2_CB(void) { g_Current_Floor_Sensor = 2; }
void Sensor_F3_CB(void) { g_Current_Floor_Sensor = 3; }
void Sensor_F4_CB(void) { g_Current_Floor_Sensor = 4; }

// =========================================================
// 3. EMERGENCY CALLBACK (PD14)
// =========================================================
void Emergency_CB(void) { g_Emergency_Flag = TRUE; }

// =========================================================
// 4. HALLWAY CALLBACKS (PC8 -> PC13) - MASTER ONLY
// =========================================================
#ifdef BUILD_AS_MASTER
void Hallway_F1_Up_CB(void)   { g_HallwayUpRequests[1]   = TRUE; }
void Hallway_F2_Up_CB(void)   { g_HallwayUpRequests[2]   = TRUE; }
void Hallway_F2_Down_CB(void) { g_HallwayDownRequests[2] = TRUE; }
void Hallway_F3_Up_CB(void)   { g_HallwayUpRequests[3]   = TRUE; }
void Hallway_F3_Down_CB(void) { g_HallwayDownRequests[3] = TRUE; }
void Hallway_F4_Down_CB(void) { g_HallwayDownRequests[4] = TRUE; }
#endif

// =========================================================
// INITIALIZATION
// =========================================================
void Button_Init(void) {
    uint8 syscfg_bus_id  = 3;   // APB2 Bus
    uint8 syscfg_bit_pos = 14;  // SYSCFG Bit position in APB2ENR

    // Equation: (BusId * 32) + BitPos
    uint8 syscfg_id = (syscfg_bus_id * 32) + syscfg_bit_pos;

    RCC_EnablePeripheral(syscfg_id);

    // 2. Cabin (Port A)
    Exti_Init(EXTI_LINE_0, EXTI_PORT_A, EXTI_EDGE_RISING, Cabin_F1_CB);
    Exti_Enable(EXTI_LINE_0);
    Exti_Init(EXTI_LINE_1, EXTI_PORT_A, EXTI_EDGE_RISING, Cabin_F2_CB);
    Exti_Enable(EXTI_LINE_1);
    Exti_Init(EXTI_LINE_2, EXTI_PORT_A, EXTI_EDGE_RISING, Cabin_F3_CB);
    Exti_Enable(EXTI_LINE_2);
    Exti_Init(EXTI_LINE_3, EXTI_PORT_A, EXTI_EDGE_RISING, Cabin_F4_CB);
    Exti_Enable(EXTI_LINE_3);

    // 3. Sensors (Port B)
    Exti_Init(EXTI_LINE_4, EXTI_PORT_B, EXTI_EDGE_RISING, Sensor_F1_CB);
    Exti_Enable(EXTI_LINE_4);
    Exti_Init(EXTI_LINE_5, EXTI_PORT_B, EXTI_EDGE_RISING, Sensor_F2_CB);
    Exti_Enable(EXTI_LINE_5);
    Exti_Init(EXTI_LINE_6, EXTI_PORT_B, EXTI_EDGE_RISING, Sensor_F3_CB);
    Exti_Enable(EXTI_LINE_6);
    Exti_Init(EXTI_LINE_7, EXTI_PORT_B, EXTI_EDGE_RISING, Sensor_F4_CB);
    Exti_Enable(EXTI_LINE_7);

    // 4. Emergency (Port D)
    Exti_Init(EXTI_LINE_14, EXTI_PORT_D, EXTI_EDGE_RISING, Emergency_CB);
    Exti_Enable(EXTI_LINE_14);

#ifdef BUILD_AS_MASTER
    // 5. Hallway (Port C)
    Exti_Init(EXTI_LINE_8,  EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F1_Up_CB);
    Exti_Enable(EXTI_LINE_8);
    Exti_Init(EXTI_LINE_9,  EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F2_Up_CB);
    Exti_Enable(EXTI_LINE_9);
    Exti_Init(EXTI_LINE_10, EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F2_Down_CB);
    Exti_Enable(EXTI_LINE_10);
    Exti_Init(EXTI_LINE_11, EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F3_Up_CB);
    Exti_Enable(EXTI_LINE_11);
    Exti_Init(EXTI_LINE_12, EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F3_Down_CB);
    Exti_Enable(EXTI_LINE_12);
    Exti_Init(EXTI_LINE_13, EXTI_PORT_C, EXTI_EDGE_RISING, Hallway_F4_Down_CB);
    Exti_Enable(EXTI_LINE_13);
#endif
}