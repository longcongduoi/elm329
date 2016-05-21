/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include "cortexm.h"
#include "GPIODrv.h"
#include "CmdUart.h"

using namespace std;

const int TxPin = 9;
const int RxPin = 10;
const int USER_AF = GPIO_AF_1;
#define RxPort  GPIOA
#define TxPort  GPIOA


/**
 * Constructor
 */
CmdUart::CmdUart()
  : txLen_(0),
    txPos_(0),
    ready_(false),
    handler_(0)
{
}

/**
 * CmdUart singleton
 */
CmdUart* CmdUart::instance()
{
    static CmdUart instance;
    return &instance;;
}

/**
 * Configure UART0
 */
void CmdUart::configure()
{
    // Enable the peripheral clock of GPIOA
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    
    // Enable the peripheral clock USART1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    // GPIO configuration for USART1 signals
    // Select AF mode on PA9 and PA10   
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = (1 << TxPin) | (1 << RxPin);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Connect PA9 to USART1_Tx, PA10 to USART1_Rx, use alternate function AF1, p165
    GPIO_PinAFConfig(TxPort, TxPin, USER_AF);
    GPIO_PinAFConfig(RxPort, RxPin, USER_AF);
}

/**
 * Use UART ROM API to configuring speed and interrupt for UART0,
 * discard the allocated UART memory block afterwards
 * @parameter[in] speed Speed to configure
 */
void CmdUart::init(uint32_t speed)
{
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStruct);

    // Enable USART
    USART1->CR1 |= USART_CR1_UE; 

    // Enable the USART Receive interrupt
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // Clear TC flag 
    USART1->ICR |= USART_ICR_TCCF;
    
    NVIC_SetPriority(USART1_IRQn, 2);
    NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * CmdUart TX handler
 */
void CmdUart::txIrqHandler()
{
    // Fill TX until full or until TX buffer is empty
    if (txPos_ < txLen_) {
        if (USART1->ISR & USART_FLAG_TXE) {
            USART1->TDR = txData_[txPos_++];
        }
    }
    else {
        USART1->ICR |= USART_ICR_TCCF;
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
    }
}

/**
 * CmdUart RX handler
 */
void CmdUart::rxIrqHandler()
{
    // Receive data, clear flag
    uint8_t ch = USART1->RDR;
    if (handler_)
        ready_ = (*handler_)(ch);
}

/**
 * CmdUart IRQ handler
 */
void CmdUart::irqHandler()
{
    // If overrun condition occurs, clear the ORE flag and recover communication
    if (USART1->ISR & USART_ISR_ORE) {
        USART1->ICR |= USART_ICR_ORECF;
    }
    
    if(USART1->ISR & USART_ISR_TC) {
        txIrqHandler();
    }
    if (USART1->ISR & USART_ISR_RXNE) {
        rxIrqHandler();
    }
}

/**
 * Send one character, blocking call for echo purposes
 * Note: Do not have USART FIFO in STM32F37x, always have to 
 * check the TXE status before sending byte
 * @parameter[in] ch Character to send
 */
void CmdUart::send(uint8_t ch) 
{
    while ((USART1->ISR & USART_FLAG_TXE) == 0)
        ;
    USART1->TDR = ch;
}

/**
 * Send the string asynch
 * @parameter[in] str String to send
 */
void CmdUart::send(const util::string& str)
{
    // wait for TX interrupt disabled when the previous transmission completed
    while (USART1->CR1 & USART_CR1_TCIE) {
        ;
    }

    // start the new transmission 
    txPos_ = 0;
    txLen_ = str.length();
    memcpy(txData_, str.c_str(), txLen_);
    if (txLen_ > 0) {
        // Initialize the transfer && Enable the USART Transmit complete interrupt
        USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        send(txData_[txPos_++]);
        //USART1->TDR = txData_[txPos_++];
    }
}

/**
 * UART1 IRQ Handler, redirect to irqHandler
 */
extern "C" void USART1_IRQHandler(void)
{
    if (CmdUart::instance())
        CmdUart::instance()->irqHandler();
}
