//
// Created by Khalaf on 29/04/2026.
//

#include "Adc.h"

#include <stddef.h>

#include "BIT_MATH.h"
#include "Adc_private.h"
#include "../Nvic/Nvic.h"
#include "../GPIO/GPIO.h"
static volatile Adc_AsyncStateType Adc_CurrentAsyncState = ADC_ASYNC_STATE_IDLE;

/*  state for async scan-group read  */
static uint16* Adc_AsyncBuf = NULL;
static uint8 Adc_AsyncTotal = 0;
static uint8 Adc_AsyncIndex = 0;
static AdcMultiChannelCallback Adc_AsyncCallback = NULL;
static AdcSingleChannelCallback Adc_SingleCallback = NULL;


static void Adc_SetSequence(uint8 SeqIndex, uint8 Channel) {
    uint8 regIdx = 2 - (SeqIndex / 6);  /* Index 0-5 -> SQR[2], 6-11 -> SQR[1] */
    uint8 bitPos = (SeqIndex % 6);
    WRITE_BIT_FIELD(ADC1->SQR[regIdx], 0x1FUL, bitPos, 5, Channel);
}
static void Adc_SetSampleTime(uint8 Channel,ADC_SampleTime_t sampleTime) {
    /* Identify Register Index: SMPR[1] for Ch 0-9, SMPR[0] for Ch 10-18 */
    uint8 regIdx = (Channel < 10) ? 1 : 0;

    /* Use the macro: MSK=0x07, IDX=(Channel%10), SIZE=3 */
    WRITE_BIT_FIELD(ADC1->SMPR[regIdx], 0x07UL, (Channel % 10), 3, sampleTime);
}



void Adc_Init(const Adc_Config_Handle_t ConfigPtr) {
    if (ConfigPtr == NULL) return;
    GPIO_PinConfig_t config;
    // 1. Prepare the configuration
    GPIO_PrepareConfig(&config, GPIO_MODE_ANALOG, GPIO_PULL_NONE, GPIO_SPEED_LOW, GPIO_OTYPE_PP);

    // 2. Define the location
    GPIO_Pin_Location_t pin = {GPIO_PORT_A, GPIO_PIN_0};

    // 3. Initialize (PIN first, then CONFIG)
    GPIO_InitPin(&pin, &config);

    WRITE_BIT_FIELD(ADC_COMMON->CCR, 0x03UL, ADC_CCR_ADCPRE_IDX, 2, ConfigPtr->prescaler);

    WRITE_BIT_FIELD(ADC1->CR[0], 0x03UL, ADC_CR1_RES, 2, ConfigPtr->resolution);

    if (ConfigPtr->groupMode >= ADC_SCAN_CH_SINGLE_CONV) {
        SET_BIT(ADC1->CR[0], ADC_CR1_SCAN);

    } else {
        CLR_BIT(ADC1->CR[0], ADC_CR1_SCAN);
    }
     SET_BIT(ADC1->CR[1], ADC_CR2_EOCS);

    /* 4. Continuous Mode Logic */
    if (ConfigPtr->groupMode == ADC_SINGLE_CH_CONT_CONV ||
        ConfigPtr->groupMode == ADC_SCAN_CH_CONT_CONV) {
        SET_BIT(ADC1->CR[1], ADC_CR2_CONT); // SHOULD BE CLEAR FOR PROTEOUS
        } else {
       CLR_BIT(ADC1->CR[1], ADC_CR2_CONT);
        }

    WRITE_BIT_FIELD(ADC1->SQR[0], 0x0FUL, 5,4,(ConfigPtr->numChannels - 1));

    for (uint8 i = 0; i < ConfigPtr->numChannels; i++) {
        /* Set thermodynamic sampling window for the LM35 */
        Adc_SetSampleTime(ConfigPtr->channels[i],ConfigPtr->sampleTime);

        /* Place the channel in the execution sequence */
        Adc_SetSequence(i, ConfigPtr->channels[i]);
    }

    /* 7. Power Enable */
    SET_BIT(ADC1->CR[1], ADC_CR2_ADON);
}
void Adc_StartConversion(void)
{
    ADC1->SR = 0;
    SET_BIT(ADC1->CR[1], ADC_CR2_SWSTART);
}

void Adc_StopConversion(void)
{
    CLR_BIT(ADC1->CR[1], ADC_CR2_ADON);
}

uint16 Adc_ReadSingleChannel(void)
{
    /* Wait for EOC */
    while (!GET_BIT(ADC1->SR, ADC_SR_EOC));

    return (uint16)(ADC1->DR & 0xFFFFU);
}

void Adc_PrepareConfig(P_void config_out, Adc_Resolution_t Resolution , Adc_Prescaler_t Prescaler, Adc_GroupMode_t GroupMode, uint8* Channels, uint8 NumChannels) {
    Adc_Config_t* pConfig = (Adc_Config_t*)config_out;

    /* Save the variables into the structure */
    pConfig->resolution  = Resolution;
    pConfig->prescaler   = Prescaler;
    pConfig->groupMode   = GroupMode;
    pConfig->channels    = Channels;
    pConfig->numChannels = NumChannels;
    pConfig->sampleTime=ADC_SMPR_84_CYCLES;
}
/* Synchronous read */
void Adc_ScanChannelGroup(uint16* Results, uint8 NumChannels)
{
    uint8 i;
    for (i = 0; i < NumChannels; i++)
    {
        while (!GET_BIT(ADC1->SR, ADC_SR_EOC))
        {
            /* poll */
        }
        Results[i] = (uint16)(ADC1->DR & 0xFFFFU);
        SET_BIT(ADC1->CR[1], ADC_CR2_ADON);

    }
}
void Adc_SetSingleCallback(AdcSingleChannelCallback Callback) {
    /* Clear scan-async state so ISR uses single path */
    Adc_AsyncBuf = 0;
    Adc_AsyncTotal = 0;
    Adc_AsyncIndex = 0;

    /* Store callback */
    Adc_SingleCallback = Callback;
    Adc_CurrentAsyncState = ADC_ASYNC_STATE_SINGLE;

    /* Enable EOC interrupt */
    SET_BIT(ADC1->CR[0], ADC_CR1_EOCIE);
    Nvic_EnableInterrupt(ADC_IRQ_NUMBER);

}
void Adc_ScanChannelGroupAsync(uint16* Results, uint8 NumChannels,
                               AdcMultiChannelCallback Callback)
{
    /* Store callback context */
    Adc_AsyncBuf = Results;
    Adc_AsyncTotal = NumChannels;
    Adc_AsyncIndex = 0;
    Adc_AsyncCallback = Callback;
    Adc_CurrentAsyncState = ADC_ASYNC_STATE_SCAN_GROUP;

    /* Enable EOC interrupt → ISR will collect each result */
    SET_BIT(ADC1->CR[0], ADC_CR1_EOCIE);
    Nvic_EnableInterrupt(ADC_IRQ_NUMBER);
   // SET_BIT(ADC1->CR[1], ADC_CR2_ADON); Potential BUG

    SET_BIT(ADC1->CR[1], ADC_CR2_SWSTART);

}
void ADC_IRQHandler(void)
{
    if (GET_BIT(ADC1->SR, ADC_SR_EOC))
    {
        if (Adc_CurrentAsyncState == ADC_ASYNC_STATE_SCAN_GROUP)
        {
            if (Adc_AsyncTotal > 0 && Adc_AsyncBuf != 0)
            {
                /* ---- Scan-group async mode ---- */
                if (Adc_AsyncIndex < Adc_AsyncTotal)
                {
                    Adc_AsyncBuf[Adc_AsyncIndex] = (uint16)(ADC1->DR & 0xFFFFU);
                    Adc_AsyncIndex++;
                }
                if (Adc_AsyncIndex >= Adc_AsyncTotal)
                {

                    // CLR_BIT(ADC1->CR[0], ADC_CR1_EOCIE);
                    // Nvic_DisableInterrupt(ADC_IRQ_NUMBER);
                   // Adc_CurrentAsyncState = ADC_ASYNC_STATE_IDLE;
                    Adc_AsyncIndex = 0;
                    if (Adc_AsyncCallback != NULL)
                    {
                        Adc_AsyncCallback(Adc_AsyncBuf, Adc_AsyncTotal);
                    }
                }
            }
            SET_BIT(ADC1->CR[1], ADC_CR2_ADON);
        }
        else if (Adc_CurrentAsyncState == ADC_ASYNC_STATE_SINGLE)
        {
            if (Adc_SingleCallback != NULL)
            {
                /*  Single-channel async mode  */
                uint16 result = (uint16)(ADC1->DR & 0xFFFFU);

                // CLEAR_BIT(ADC1->CR1, CR1_EOCIE);
                ADC1->SR = 0;

                Adc_SingleCallback(result);
            }
        }
        else
        {
            // error or idle
            ADC1->SR = 0;
        }
    }
}