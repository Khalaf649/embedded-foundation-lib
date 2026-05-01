//
// Created by Khalaf on 29/04/2026.
//

#include "lm35.h"

#include <stddef.h>

static Adc_Config_t Lm35_AdcConfig;
static Adc_ChannelConfig_t Lm35_ChannelMap[1];       /* single-element array for Adc API  */
static Lm35_Callback_t Lm35_UserCallback = NULL;  /* stored async callback             */

static float Lm35_Convert(uint16 RawAdc)
{
    float voltage_mv = ((float)RawAdc * LM35_VREF_MV) / LM35_ADC_MAX;
    return voltage_mv / LM35_MV_PER_DEG ;
}

void Lm35_Init(GPIO_Port_t Port, GPIO_Pin_t Pin, ADC_Channel_t Channel,
               Adc_Prescaler_t Prescale, Adc_Resolution_t Resolution)
{
    /* 1. Map the physical pin to the ADC channel */
    Lm35_ChannelMap[0].port       = Port;
    Lm35_ChannelMap[0].pin        = Pin;
    Lm35_ChannelMap[0].channel_id = Channel;

    /* 2. Prepare the master ADC configuration */
    /* Updated signature to pass the struct array and proper pointer type */
    Adc_PrepareConfig(&Lm35_AdcConfig,
                      Resolution,
                      Prescale,
                      ADC_SCAN_CH_CONT_CONV,
                      Lm35_ChannelMap,
                      1);

    /* 3. Hand off to the ADC driver for hardware setup */
    Adc_Init(&Lm35_AdcConfig);
}
float Lm35_GetTemperature(void)
{
    Adc_StartConversion();
    uint16 raw = Adc_ReadSingleChannel();
    return Lm35_Convert(raw);
}
static void Lm35_AdcInternalCallback(uint16 RawAdc)
{
    if (Lm35_UserCallback != NULL)
    {
        /* Convert here so the user callback signature is just float */
        Lm35_UserCallback(Lm35_Convert(RawAdc));
    }
    Adc_StartConversion();
}
void Lm35_GetTemperatureAsync(Lm35_Callback_t Callback)
{
    Lm35_UserCallback = Callback;

    Adc_SetSingleCallback(Lm35_AdcInternalCallback);

}