//
// Created by Khalaf on 13/04/2026.
//

#ifndef SEVENSEG_H
#define SEVENSEG_H
#define SEVENSEG_PIN_COUNT  7
#include "GPIO.h"

typedef enum {
    SEVENSEG_COMMON_CATHODE = 0,
    SEVENSEG_COMMON_ANODE
} SevenSeg_Type_t;
typedef struct {
    GPIO_Pin_Location_t segments[SEVENSEG_PIN_COUNT]; // A, B, C, D, E, F, G
    SevenSeg_Type_t      type;
} SevenSeg_Config_t;
typedef  SevenSeg_Config_t* SevenSeg_Config_Handle_t;
void SevenSeg_PrepareConfig(P_void config_out, GPIO_Pin_Location_t segments[SEVENSEG_PIN_COUNT], SevenSeg_Type_t type);
void SevenSeg_Init(const SevenSeg_Config_Handle_t config_h);
void SevenSeg_WriteDigit(const SevenSeg_Config_Handle_t config_h, uint8 digit);
void SevenSeg_Clear(const SevenSeg_Config_Handle_t config_h);


#endif //SEVENSEG_H
