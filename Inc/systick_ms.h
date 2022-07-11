#ifndef _SYSTICK_MS_H
#define _SYSTICK_MS_H

#include <stm32f4xx.h>
#include <stdint.h>

extern volatile uint32_t _ticks;

#define CUR_TICKS   (_ticks)
#define millis()  (_ticks)

void WaitMs(uint32_t ms);
void InitSystickDefault(void);
void InitSystick(uint32_t intervalMs);

#endif
