#ifndef _APA102_RGB_H
#define _APA102_RGB_H

#include "nucleo_board.h"
#include <math.h>

#pragma pack(1)
typedef struct rgb_struct
{
    uint8_t alpha;                            // bytes order for APA_102
    uint8_t b;
    uint8_t g;
    uint8_t r;
} RGB_LED;

#define RAINBOW_COLORS_COUNT  32

void RGB_Rainbow_32_Generate(RGB_LED *led_array);
void RGB_Set_SetIntesity(RGB_LED *led_array, int count, uint8_t val);

bool APA_102_init(void);
// void SPISend32(uint32_t val);
void APA_102_SendRGB(RGB_LED *ledArray, int lenArray, int offset, int countLED);

#endif // _APA102_RGB_H
