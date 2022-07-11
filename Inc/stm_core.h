/*
 * stm_core.h
 *
 *  Created on: 6. 2. 2017
 *      Author: Petr
 */

#ifndef STM_CORE_H_
#define STM_CORE_H_

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"

//TODO implement type/board switch
typedef enum { clockSourceHSI, clockSourceHSE } eClockSources;
bool SystemClock_100MHz(eClockSources clkSrc);

typedef enum {
  ioPortOutputPP,       // output Push-Pull
  ioPortOutputOC,       // output Open Collector/Drain
  ioPortAnalog,         // analogo input - required for A/D
  ioPortInputFloat,     // input without pull-up/down
  ioPortInputPU,        // input with pull-up
  ioPortInputPD,        // input with pull-down
  ioPortAlternatePP,    // alternate output - push/pull
  ioPortAlternateOC     // alternate output - open drain
} eIoPortModes;

bool STM_SetPinGPIO(GPIO_TypeDef *gpio, uint32_t bitnum, eIoPortModes mode);
bool STM_SetAFGPIO(GPIO_TypeDef *gpio, uint32_t bitnum, uint32_t afValue);

void GPIOToggle(GPIO_TypeDef *gpio, uint32_t bitnum);
bool GPIORead(GPIO_TypeDef *gpio, uint32_t bitnum);
void GPIOWrite(GPIO_TypeDef *gpio, uint32_t bitnum, bool state);

typedef enum { busClockAHB,
  busClockAPB1, busClockAPB2,
  timersClockAPB1, timersClockAPB2 } eBusClocks;
uint32_t STM_GetBusClock(eBusClocks clk);

uint32_t STM_GetTimerClock(int timerNum);

typedef struct GPIO_init_type_struct
{
  GPIO_TypeDef *gpio;
  uint32_t bitnum;
  eIoPortModes mode;
  uint32_t afValue;
} GPIO_init_type;

#endif /* STM_CORE_H_ */
