//
// Created by Khalaf on 29/04/2026.
//

#include "lm35.h"

#include <stddef.h>

static Adc_Config_t Lm35_AdcConfig;
static uint8           Lm35_ChannelArr[1];        /* single-element array for Adc API  */
static Lm35_Callback_t Lm35_UserCallback = NULL;  /* stored async callback             */

static float Lm35_Convert(uint16 RawAdc)
{
    float voltage_mv = ((float)RawAdc * LM35_VREF_MV) / LM35_ADC_MAX;
    return voltage_mv / LM35_MV_PER_DEG -48;
}

void  Lm35_Init(uint8 Channel, Adc_Prescaler_t Prescale, Adc_Resolution_t Resolution) {
    Lm35_ChannelArr[0] = 0;

    Adc_PrepareConfig(&Lm35_AdcConfig,
                      ADC_RES_12BIT,
                      ADC_PRESCALER_DIV2,
                      ADC_SCAN_CH_CONT_CONV,
                      Lm35_ChannelArr,
                      1);
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
}
void Lm35_GetTemperatureAsync(Lm35_Callback_t Callback)
{
    Lm35_UserCallback = Callback;

    Adc_SetSingleCallback(Lm35_AdcInternalCallback);

    Adc_StartConversion();
}