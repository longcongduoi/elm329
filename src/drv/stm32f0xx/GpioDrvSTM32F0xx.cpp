/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "cortexm.h"
#include "GpioDrv.h"

static GPIO_TypeDef* const GPIOPtr[]  = { GPIOA, GPIOB, GPIOC };

/**
 * Enabling GPIOx clock
 * @param[in] portNum GPIO number (0..7)
 */
void GPIOConfigure(uint32_t portNum)
{
    RCC->AHBENR |= (1UL << (portNum + 17));
}

/**
 * Setting GPIO pin direction
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] dir GPIO_DIR_INPUT, GPIO_DIR_OUTPUT
 */
void GPIOSetDir(uint32_t portNum, uint32_t pinNum, uint32_t dir)
{
    GPIO_TypeDef* gpio = GPIOPtr[portNum];
    if (dir == GPIO_OUTPUT) {
        gpio->ODR     &= ~(1UL << 8);          // Output data, set to 0
        gpio->MODER   &= ~(3UL << 2 * pinNum);
        gpio->MODER   |= (1UL << 2 * pinNum);  // Output mode
        gpio->OTYPER  &= ~(1UL << pinNum);     // Output push-pull
        gpio->OSPEEDR |= (3UL << 2 * pinNum);  // Port output 50 MHz High speed
        gpio->PUPDR   &= ~(3UL << 2 * pinNum); // No pull-up, pull-down
    }
    else {
        gpio->MODER &= ~(3UL << 2 * pinNum);   // Input mode
        gpio->PUPDR &= ~(3UL << 2 * pinNum);   // No pull-up, pull-down
    }
}

/**
 * Setting the port pin value
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] val Port pin value (0 or 1)
 */
void GPIOPinWrite(uint32_t portNum, uint32_t pinNum, uint32_t val)
{
    GPIOPtr[portNum]->BSRR = val ? (1UL << pinNum) : (1UL << (pinNum + 16));
}

/**
 * Read port pin
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @return pin value (0 or 1)
 */
uint32_t GPIOPinRead (uint32_t portNum, uint32_t pinNum)
{
    return (GPIOPtr[portNum]->IDR & (1UL << pinNum)) ? 1 : 0;
}


/**
 * Setting CPU-specific port attributes, like open drain and etc.
 * @param[in] portNum GPIO number (0..7)
 * @param[in] pinNum Port pin number
 * @param[in] val Port attribute
 */
void GPIOPinConfig(uint32_t portNum, uint32_t pinNum, uint32_t val)
{
    GPIO_TypeDef* gpio = GPIOPtr[portNum];
    if (val == GPIO_OPEN_DRAIN) {
        gpio->OTYPER |= (1UL << pinNum);
    }
}
