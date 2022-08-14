#ifndef _MAX7219_LEDS_H
#define _MAX7219_LEDS_H

#include "nucleo_board.h"

bool MAX7219_Init(void);

bool MAX7219_ViewHEX(uint16_t x);
bool MAX7219_ViewDEC(uint16_t x);
bool MAX7219_SetIntensity(uint8_t b);
bool MAX7219_BitContent(uint8_t *pData);

#endif // _MAX7219_LEDS_H
