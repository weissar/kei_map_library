#ifndef _OLED_1305_H
#define _OLED_1305_H

#include "display_hilevel.h"

bool OLED_1305_Init(void);
bool OLED_1305_UpdateContent(void);

bool OLED_1305_Fill(uint8_t bFill);
bool OLED_1305_DemoCntRow(uint8_t bRow);

#endif  // _OLED_1305_H
