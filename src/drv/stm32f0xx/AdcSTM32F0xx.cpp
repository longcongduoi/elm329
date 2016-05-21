/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "cortexm.h"
#include "AdcDriver.h"

const uint16_t AdcPin     = GPIO_Pin_0;
const uint32_t AdcChannel = ADC_Channel_8;
const GPIO_TypeDef* GPIOx = GPIOB;

/**
 * Configuring comparator
 */
void AdcDriver::configure()
{
    // Configure the ADC clock
    RCC_PCLKConfig(ADC_ClockMode_SynClkDiv2);

    // Enable ADC1 clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    // GPIOA Periph clock enableed in PalGPIO_InitClock()
    
    // Configure ADC Channel0 as analog input
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = AdcPin ;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(const_cast<GPIO_TypeDef*>(GPIOB), &GPIO_InitStruct);

    // Initialize ADC structure
    ADC_InitTypeDef ADC_InitStruct;
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // Trigger detection disabled
    ADC_InitStruct.ADC_ExternalTrigConv = 0; // Do not care  
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_Init(ADC1, &ADC_InitStruct);
    
    // Convert the ADC1 Channel 0 with 239.5 Cycles as sampling time 
    ADC_ChannelConfig(ADC1, AdcChannel, ADC_SampleTime_239_5Cycles);   
}

uint32_t AdcDriver::read()
{
    // ADC Calibration
    ADC_GetCalibrationFactor(ADC1);
      
    // Enable ADC
    ADC_Cmd(ADC1, ENABLE);
  
    // Start ADC1 Software Conversion
    ADC_StartOfConversion(ADC1);    
  
    // Test EOC flag
    while(!ADC_GetFlagStatus(ADC1, ADC_ISR_EOC));
    
    // Get ADC1 converted data
    uint16_t adcValue = ADC_GetConversionValue(ADC1);
    
    ADC_Cmd(ADC1, DISABLE);
    return adcValue;
}
