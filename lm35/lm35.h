//
// Created by Khalaf on 29/04/2026.
//

#ifndef LM35_H
#define LM35_H
#define LM35_VREF_MV      3300.0f   /* supply / reference voltage in mV  */
#define LM35_ADC_MAX      4096.0f   /* 12-bit full scale                  */
#define LM35_MV_PER_DEG   10.0f     /* LM35 sensitivity: 10 mV per °C     */
#include "STD_TYPES.h"
#include "../Adc/Adc.h"

typedef void (*Lm35_Callback_t)(float Temperature);


void  Lm35_Init(uint8 Channel, Adc_Prescaler_t Prescale, Adc_Resolution_t Resolution);

float Lm35_GetTemperature(void);


void  Lm35_GetTemperatureAsync(Lm35_Callback_t Callback);
#endif //LM35_H
