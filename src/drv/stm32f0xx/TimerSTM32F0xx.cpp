/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "cortexm.h"
#include "Timer.h"

const uint16_t tickDiv = (SystemCoreClock / 1000);
static TIM_TypeDef*  TimerPtr[] = { TIM3, TIM14 };

/**
 * Configuring timers
 */
void Timer::configure()
{
    // Enable timer clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;
}

/**
 * Construct the Timer object
 * @param[in] timerNum Logical timer number (0..2)
 */
Timer::Timer(int timerNum)
{
    timer_ = TimerPtr[timerNum];
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
    TIM_TimeBaseStruct.TIM_Period = 0xFFFF;           // Autoload register
    TIM_TimeBaseStruct.TIM_Prescaler = (tickDiv - 1); // Divide to 1ms
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_CounterMode = (TIM_CounterMode_Up | TIM_OPMode_Single);
    TIM_TimeBaseInit(timer_, &TIM_TimeBaseStruct);
}

/**
 * Start/restart the timer, interval <= 349 ms
 * @param[in] interval Timer interval in milliseconds
 */
void Timer::start(uint32_t interval) 
{
    timer_->ARR = interval;
    timer_->CNT = 0;
    timer_->SR  = 0; // Clear the flags
    timer_->CR1 |= TIM_CR1_CEN; // Enable the TIMn timer
}

/**
 * Check if timer is still running
 * @return Timer interrupt status (false or true)
 */
bool Timer::isExpired() const
{
    return (timer_->SR & TIM_FLAG_Update);
}

/**
 * Factory method to construct the Timer object
 * @param[in] timerNum Logical timer number (0..1)
 * @return Timer pointer
 */
Timer* Timer::instance(int timerNum)
{
    static Timer timer0(0);
    static Timer timer1(1);
    
    switch (timerNum) {
      case Timer::TIMER0:
          return &timer0;
      
      case Timer::TIMER1: 
          return &timer1;
        
      default:
        return 0;
    }
}

static PeriodicCallbackT irqCallback;

extern "C" void TIM16_IRQHandler(void)
{
    if (TIM16->SR & TIM_FLAG_Update) {
        TIM16->SR &= ~TIM_FLAG_Update; // Clear TIM16 update interrupt
        if (irqCallback) {
            (*irqCallback)();
        }
    }
}

/**
 * Construct the PeriodicTimer instance
 * @param[in] callback Timer callback handler
 */
PeriodicTimer::PeriodicTimer(PeriodicCallbackT callback)
{
    irqCallback = callback;
    
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
    TIM_TimeBaseStruct.TIM_Period = 0xFFFF;           // Autoload register
    TIM_TimeBaseStruct.TIM_Prescaler = (tickDiv - 1); // Divide to 1ms
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_CounterMode = (TIM_CounterMode_Up | TIM_OPMode_Repetitive);
    TIM_TimeBaseInit(TIM16, &TIM_TimeBaseStruct);
    TIM16->DIER |= TIM_IT_Update;
    NVIC_EnableIRQ(TIM16_IRQn);
}

/**
 * Start/restart the timer
 * @param[in] interval Timer interval in milliseconds
 */
void PeriodicTimer::start(uint32_t interval)
{
    TIM16->ARR = interval;
    TIM16->CNT = 0;
    TIM16->SR  = 0; // Clear the flags
    TIM16->CR1 |= TIM_CR1_CEN; // Enable the TIM16 time
}

/**
 *  Stop the timer
 */
void PeriodicTimer::stop()
{
    TIM16->CR1 &= ~TIM_CR1_CEN;
}
