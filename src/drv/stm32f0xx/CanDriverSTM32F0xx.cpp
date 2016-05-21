/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <cstdlib>
#include <adaptertypes.h>
#include "cortexm.h"
#include "CanDriver.h"
#include "GPIODrv.h"
#include <canmsgbuffer.h>
#include <led.h>

using namespace std;

// The adapter specifics
const int CanTxPin  = 12;
const int CanRxPin  = 11;
const int CanRxPort = 0;
const int CanTxPort = 0;
const int CAN_AF = GPIO_AF_4;

const int CAN_PRESCALER = 6 ; // For bus clock 48Mhz
const uint32_t FMR_FINIT = 0x00000001;
const uint32_t MCR_DBF   = 0x00010000;
static GPIO_TypeDef* const GPIOPtr[] = { GPIOA, GPIOB, GPIOC };


// FIFO stuff
const int FIFO_NUM = 10;
static CanRxMsg RxFifo[FIFO_NUM];
static volatile uint32_t RxFifoFlag;
static volatile uint32_t FifoReadPos;
static volatile uint32_t FifoWritePos;


extern "C" void CEC_CAN_IRQHandler(void)
{
    // Blink LED from here, when RX operation is completed
    AdptLED::instance()->blinkRx();

    CanRxMsg* msg = &RxFifo[FifoWritePos];
    CAN_Receive(CAN, CAN_FIFO0, msg);

    // Advance the FIFO next writing position
    uint32_t mask = 0x1 << FifoWritePos;
    RxFifoFlag |= mask;
    FifoWritePos = (FifoWritePos == FIFO_NUM-1) ? 0 : FifoWritePos + 1;
}

/**
 * Configure I/O pins as CAN driver
 */
static void configureCANPins()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = (1 << CanTxPin) | (1 << CanRxPin);
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * Configuring CanDriver
 */
void CanDriver::configure()
{
    // Enable the clock for the CAN
    RCC->APB1ENR |= RCC_APB1Periph_CAN;

    // Connect PA12 to CAN_Tx, PA11 to CAN_Rx, use alternate function AF4
    GPIO_PinAFConfig(GPIOPtr[CanRxPort], CanRxPin, CAN_AF);
    GPIO_PinAFConfig(GPIOPtr[CanTxPort], CanTxPin, CAN_AF);

    // Configure these CAN pins in alternate function mode
    configureCANPins();

    CAN_InitTypeDef CAN_InitStruct;
    CAN_DeInit(CAN);
    CAN_StructInit(&CAN_InitStruct);
    CAN_InitStruct.CAN_Prescaler = CAN_PRESCALER; // Specifies the length of a time quantum. It ranges from 1 to 1024.
    CAN_InitStruct.CAN_Mode = CAN_Mode_Normal; // Specifies the CAN operating mode.
    CAN_InitStruct.CAN_SJW = CAN_SJW_3tq;      // Specifies the synchronization jump
    CAN_InitStruct.CAN_BS1 = CAN_BS1_12tq;     // Specifies the number of time quanta in Bit Segment 1.
    CAN_InitStruct.CAN_BS2 = CAN_BS2_3tq;      // Specifies the number of time quanta in Bit Segment 2.
    CAN_InitStruct.CAN_TTCM = DISABLE;         // Enable or disable the time triggered communication mode.
    CAN_InitStruct.CAN_ABOM = ENABLE;          // Enable or disable the automatic bus-off management.
    CAN_InitStruct.CAN_AWUM = DISABLE;         // Enable or disable the automatic wake-up mode.
    CAN_InitStruct.CAN_NART = DISABLE;         // Enable or disable the non-automatic retransmission mode.
    CAN_InitStruct.CAN_RFLM = DISABLE;         // Enable or disable the Receive FIFO Locked mode.
    CAN_InitStruct.CAN_TXFP = DISABLE;         // Enable or disable the transmit FIFO priority.
    CAN_Init(CAN, &CAN_InitStruct);

    // Enable FIFO 0 message pending Interrupt
    CAN_ITConfig(CAN, CAN_IT_FMP0, ENABLE);

    CAN->ESR = 0; 
    
    // Disable Debug Freeze
    CAN->MCR &= ~MCR_DBF;
}

/**
 * CanDriver singleton
 * @return The pointer to CandRiver instance
 */
CanDriver* CanDriver::instance()
{
    static CanDriver instance;
    return &instance;
}

/**
 * Intialize the CAN controller and interrupt handler
 */
CanDriver::CanDriver()
{
    // Enable the CAN Interrupt
    NVIC_SetPriority(CEC_CAN_IRQn, 0);
    NVIC_EnableIRQ(CEC_CAN_IRQn);
}

/**
 * Transmits a sequence of bytes to the ECU over CAN bus
 * @parameter   buff   CanMsgBuffer instance
 * @return the send operation completion status
 */
bool CanDriver::send(const CanMsgBuffer* buff)
{   
    // Blink LED from here, when TX operation is completed
    AdptLED::instance()->blinkTx();

    CanTxMsg msg;
    msg.StdId = msg.ExtId = buff->id;
    msg.IDE   = buff->extended ? CAN_ID_EXT : CAN_ID_STD;
    msg.RTR   = CAN_RTR_Data;
    msg.DLC   = buff->dlc;
    memcpy(msg.Data, buff->data, 8);
    uint8_t val = CAN_Transmit(CAN, &msg);
    return (val != CAN_TxStatus_NoMailBox);
}

/**
 * Function Name: setFilter()
 * Description:   Set the CAN filter for FIFO buffer
 * Returns:       None
 */
const uint32_t filterNum = 0;
static void setFilter(uint32_t val, bool extended)
{
    const uint32_t filterNumberBitPos = 1 << filterNum;
    
    // Initialisation mode for the filter
    CAN->FMR |= FMR_FINIT;

    // Filter Deactivation
    CAN->FA1R &= ~filterNumberBitPos;

    // 32-bit scale for the filter
    CAN->FS1R |= filterNumberBitPos;

    // 32-bit identifier 
    // STDID[10:0], EXTID[17:0], IDE and RTR bits.
    CAN->sFilterRegister[filterNum].FR1 = extended ? ((val << 3) | 0x0000004) : (val << 21);

    // FIFO 0 assignation for the filter
    CAN->FFA1R &= ~(uint32_t)filterNumberBitPos;

    // Filter activation
    CAN->FA1R |= filterNumberBitPos;

    // Leave the initialisation mode for the filter
    CAN->FMR &= ~FMR_FINIT;
}

/**
 * Function Name: setMask()
 * Description:   Set the CAN mask for FIFO buffer
 * Returns:       None
 */
static void setMask(uint32_t val, bool extended)
{
    const uint32_t filterNumberBitPos = 1 << filterNum;
    
    // Initialisation mode for the filter
    CAN->FMR |= FMR_FINIT;

    // Filter Deactivation
    CAN->FA1R &= ~filterNumberBitPos;

    // 32-bit mask
    // STDID[10:0], EXTID[17:0], IDE and RTR bits.
    CAN->sFilterRegister[filterNum].FR2 = extended ? ((val << 3) | 0x0000004) : (val << 21);

    // Id/Mask mode for the filter
    CAN->FM1R &= ~filterNumberBitPos;

    // Filter activation
    CAN->FA1R |= filterNumberBitPos;

    // Leave the initialisation mode for the filter
    CAN->FMR &= ~FMR_FINIT;
}

/**
 * Set the CAN filter for FIFO buffer
 * @parameter   filter    CAN filter value
 * @parameter   mask      CAN mask value
 * @parameter   extended  CAN extended message flag
 * @return  the operation completion status
 */
bool CanDriver::setFilterAndMask(uint32_t filter, uint32_t mask, bool extended)
{
    setFilter(filter, extended);
    setMask(mask, extended);
    return true;
}

/**
 * Read the CAN frame from FIFO buffer
 * @return  true if read the frame / false if no frame
 */
bool CanDriver::read(CanMsgBuffer* buff)
{ 
    if (RxFifoFlag) {
        const CanRxMsg* msg = &RxFifo[FifoReadPos];
        buff->id = msg->IDE ? msg->ExtId : msg->StdId;
        buff->extended = (msg->IDE == CAN_ID_EXT);
        buff->dlc = msg->DLC;
        memcpy(buff->data, msg->Data, 8);

        // Advance the FIFO next reading position
        uint32_t mask = 0x1 << FifoReadPos;

        CAN_ITConfig(CAN, CAN_IT_FMP0, DISABLE);
        RxFifoFlag &= ~mask;
        CAN_ITConfig(CAN, CAN_IT_FMP0, ENABLE);
        FifoReadPos = (FifoReadPos == FIFO_NUM-1) ? 0 : FifoReadPos + 1;        
        return true;
    }
    else {
        return false;
    }
}

/**
 * Read CAN frame received status
 * @return  true/false
 */
bool CanDriver::isReady() const
{
     return (RxFifoFlag != 0x0);
}

/**
 * Wakes up the CAN peripheral from sleep mode
 * @return  true/false
 */
bool CanDriver::wakeUp()
{
    return CAN_WakeUp(CAN) == CAN_WakeUp_Ok;
}

/**
 * Enters the sleep (low power) mode
 * @return  true/false
 */
bool CanDriver::sleep()
{
    return false;
}

/**
 * Switch on/off CAN and let the CAN pins controlled directly (testing mode)
 * @parameter  val  CAN testing mode flag 
 */
void CanDriver::setBitBang(bool val)
{
    if (!val) {
        configureCANPins();
    }
    else {
        GPIOSetDir(CanRxPort, CanRxPin, GPIO_INPUT);
        GPIOSetDir(CanTxPort, CanTxPin, GPIO_OUTPUT);
    }
}

/**
 * Set the CAN transmitter pin status
 * @parameter  bit  CAN TX pin value
 */
void CanDriver::setBit(uint32_t bit)
{
    GPIOPinWrite(CanTxPort, CanTxPin, bit);
}

/**
 * Read CAN RX pin status
 * @return pin status, 1 if set, 0 otherwise
 */
uint32_t CanDriver::getBit()
{
    return GPIOPinRead(CanRxPort, CanRxPin);
}
