//
// Created by Khalaf on 29/04/2026.
//

#ifndef ADC_H
#define ADC_H
#include "Adc_private.h"
#include "STD_TYPES.h"
typedef enum {
    ADC_RES_12BIT = 0,
    ADC_RES_10BIT = 1,
    ADC_RES_8BIT  = 2,
    ADC_RES_6BIT  = 3
} Adc_Resolution_t;

typedef enum {
    ADC_PRESCALER_DIV2 = 0,
    ADC_PRESCALER_DIV4 = 1,
    ADC_PRESCALER_DIV6 = 2,
    ADC_PRESCALER_DIV8 = 3
} Adc_Prescaler_t;

typedef enum {
    ADC_SINGLE_CH_SINGLE_CONV = 0,
    ADC_SINGLE_CH_CONT_CONV,
    ADC_SCAN_CH_SINGLE_CONV,
    ADC_SCAN_CH_CONT_CONV
} Adc_GroupMode_t;

/* The Master Config */
typedef struct {
    Adc_Resolution_t resolution;
    Adc_GroupMode_t  groupMode;       /* The 4 modes */
    Adc_Prescaler_t   prescaler;
    uint8* channels;        /* Array of channels */
    uint8     numChannels;
    ADC_SampleTime_t sampleTime;     /* Sample time for all channels */
} Adc_Config_t;

typedef enum {
    ADC_CHANNEL_0 = 0U,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_9,
    ADC_CHANNEL_10,
    ADC_CHANNEL_11,
    ADC_CHANNEL_12,
    ADC_CHANNEL_13,
    ADC_CHANNEL_14,
    ADC_CHANNEL_15
} ADC_Channel_t;

typedef Adc_Config_t* Adc_Config_Handle_t;

/* Callback types for async operations */
typedef void (*AdcMultiChannelCallback)(uint16 *Results, uint8 NumChannels);
typedef void (*AdcSingleChannelCallback)(uint16 Result);

#define ADC0_CHANNEL 0


void Adc_Init(const Adc_Config_Handle_t ConfigPtr);

void Adc_StartConversion(void);

void Adc_StopConversion(void);

uint16 Adc_ReadSingleChannel(void);

void Adc_PrepareConfig(P_void config_out, Adc_Resolution_t Resolution , Adc_Prescaler_t Prescaler, Adc_GroupMode_t GroupMode, uint8* Channels, uint8 NumChannels);
void Adc_ScanChannelGroup(uint16* Results, uint8 NumChannels);
void Adc_SetSingleCallback(AdcSingleChannelCallback Callback);
void Adc_ScanChannelGroupAsync(uint16* Results, uint8 NumChannels,
                               AdcMultiChannelCallback Callback);

#endif //ADC_H
