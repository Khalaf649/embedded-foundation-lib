// exti.h
#ifndef EXTI_H_
#define EXTI_H_



typedef enum {
    EXTI_LINE_0  = 0U,
    EXTI_LINE_1  = 1U,
    EXTI_LINE_2  = 2U,
    EXTI_LINE_3  = 3U,
    EXTI_LINE_4  = 4U,
    EXTI_LINE_5  = 5U,
    EXTI_LINE_6  = 6U,
    EXTI_LINE_7  = 7U,
    EXTI_LINE_8  = 8U,
    EXTI_LINE_9  = 9U,
    EXTI_LINE_10 = 10U,
    EXTI_LINE_11 = 11U,
    EXTI_LINE_12 = 12U,
    EXTI_LINE_13 = 13U,
    EXTI_LINE_14 = 14U,
    EXTI_LINE_15 = 15U
} EXTI_Line_t;

typedef enum {
    EXTI_PORT_A = 0U,
    EXTI_PORT_B = 1U,
    EXTI_PORT_C = 2U,
    EXTI_PORT_D = 3U,
    EXTI_PORT_E = 4U
} EXTI_Port_t;

/* EXTI Trigger Edge Types */
typedef enum {
    EXTI_EDGE_RISING  = 0U,
    EXTI_EDGE_FALLING = 1U,
    EXTI_EDGE_BOTH    = 2U
} EXTI_Edge_t;

/* Callback Type */
typedef void (*ExtiCallback)(void);


void Exti_Init(EXTI_Line_t LineNumber, EXTI_Port_t PortName,
               EXTI_Edge_t EdgeType, ExtiCallback Callback);
void Exti_Enable(EXTI_Line_t LineNumber);

void Exti_Disable(EXTI_Line_t LineNumber);

#endif /* EXTI_H_ */