//
// Created by Khalaf on 29/04/2026.
//

#ifndef ADC_PRIVATE_H
#define ADC_PRIVATE_H
#include "STD_TYPES.h"
typedef struct {
    volatile uint32 SR;          /* 0x00 - Status register                 */
    volatile uint32 CR[2];         /* 0x04  - Control register  0x08 - Control register 2 */
    volatile uint32 SMPR[2];       /* 0x0C - Sample time register 1 (ch10-18) 0x10 - Sample time register 2 (ch0-9)*/
    volatile uint32 JOFR[4];       /* 0x14 - Injected channel offset 1      */
    volatile uint32 HTR;         /* 0x24 - Watchdog higher threshold       */
    volatile uint32 LTR;         /* 0x28 - Watchdog lower threshold        */
    volatile uint32 SQR[3];        /* 0x2C - Regular sequence register 1     */
    volatile uint32 JSQR;        /* 0x38 - Injected sequence register      */
    volatile uint32 JDR[4];        /* 0x3C - Injected data register 1        */
    volatile uint32 DR;          /* 0x4C - Regular data register           */
} AdcType;

typedef struct {
    volatile uint32 CSR;  /* 0x00 - Common status register       */
    volatile uint32 CCR;  /* 0x04 - Common control register      */
    volatile uint32 CDR;  /* 0x08 - Common regular data register */
} AdcCommonType;
#define ADC1_BASE         0x40012000UL
#define ADC_COMMON_BASE   (ADC1_BASE+0x300UL)

// Cast the addresses to your structs
#define ADC1         ((AdcType *)ADC1_BASE)
#define ADC_COMMON   ((AdcCommonType *)ADC_COMMON_BASE)

typedef enum {
    ADC_ASYNC_STATE_IDLE       = 0U,
    ADC_ASYNC_STATE_SINGLE     = 1U,
    ADC_ASYNC_STATE_SCAN_GROUP = 2U
} Adc_AsyncStateType;

typedef enum {
    ADC_SR_AWD   = 0U,  /* Analog watchdog flag             */
    ADC_SR_EOC   = 1U,  /* End of conversion               */
    ADC_SR_JEOC  = 2U,  /* Injected end of conversion      */
    ADC_SR_JSTRT = 3U,  /* Injected channel start flag     */
    ADC_SR_STRT  = 4U,  /* Regular channel start flag      */
    ADC_SR_OVR   = 5U   /* Overrun                         */
} ADC_SR_Bit_t;

typedef enum {
    ADC_CR1_EOCIE  = 5U,  /* EOC interrupt enable            */
    ADC_CR1_JEOCIE = 7U,  /* Injected EOC interrupt enable   */
    ADC_CR1_SCAN   = 8U,  /* Scan mode                       */
    ADC_CR1_RES    = 12U  /* Resolution (2 bits: 24-25)      */
} ADC_CR1_Bit_t;

typedef enum {
    ADC_CR2_ADON    = 0U,  /* A/D converter ON                */
    ADC_CR2_CONT    = 1U,  /* Continuous conversion           */
    ADC_CR2_DMA     = 8U,  /* DMA mode enable                 */
    ADC_CR2_EOCS    = 10U, /* End of conversion selection     */
    ADC_CR2_SWSTART = 30U  /* Start conversion of regular channels */
} ADC_CR2_Bit_t;

typedef enum {
    ADC_CCR_ADCPRE_IDX  = 8U,   /* (8 * 2 = Bit 16) Field Size = 2 */
    ADC_CCR_VBATE       = 22U,
    ADC_CCR_TSVREFE     = 23U
} ADC_CCR_Reg_t;

typedef enum {
    ADC_SMPR_3_CYCLES   = 0x00U,
    ADC_SMPR_15_CYCLES  = 0x01U,
    ADC_SMPR_28_CYCLES  = 0x02U,
    ADC_SMPR_56_CYCLES  = 0x03U,
    ADC_SMPR_84_CYCLES  = 0x04U,
    ADC_SMPR_112_CYCLES = 0x05U,
    ADC_SMPR_144_CYCLES = 0x06U,
    ADC_SMPR_480_CYCLES = 0x07U
} ADC_SampleTime_t;
#define ADC_IRQ_NUMBER    18U



#endif //ADC_PRIVATE_H
