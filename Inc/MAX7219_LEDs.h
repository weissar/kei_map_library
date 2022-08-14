#ifndef _MAX7219_LEDS_H
#define _MAX7219_LEDS_H

#include "nucleo_board.h"

bool MAX7219_Init(void);

bool MAX7219_ViewHEX(uint16_t x);
bool MAX7219_ViewDEC(uint16_t x);
bool MAX7219_SetIntensity(uint8_t b);
bool MAX7219_BitContent(uint8_t *pData);

uint8_t MAX7219_DecodeSeg(uint8_t val);
uint8_t MAX7219_AddPoint(uint8_t val);

#endif // _MAX7219_LEDS_H
