#include "stm_i2c.h"          // obsahuje i CORE apod.
#include "OLED_1305.h"

// https://github.com/adafruit/Adafruit_SSD1305_Library
#define SSD1305_LCDWIDTH                  128
#define SSD1305_LCDHEIGHT                 64

#define SSD1305_SETLOWCOLUMN 0x00
#define SSD1305_SETHIGHCOLUMN 0x10
#define SSD1305_MEMORYMODE 0x20
#define SSD1305_SETCOLADDR 0x21
#define SSD1305_SETPAGEADDR 0x22
#define SSD1305_SETSTARTLINE 0x40

#define SSD1305_SETCONTRAST 0x81
#define SSD1305_SETBRIGHTNESS 0x82

#define SSD1305_SETLUT 0x91

#define SSD1305_SEGREMAP 0xA0
#define SSD1305_DISPLAYALLON_RESUME 0xA4
#define SSD1305_DISPLAYALLON 0xA5
#define SSD1305_NORMALDISPLAY 0xA6
#define SSD1305_INVERTDISPLAY 0xA7
#define SSD1305_SETMULTIPLEX 0xA8
#define SSD1305_DISPLAYDIM 0xAC
#define SSD1305_MASTERCONFIG 0xAD
#define SSD1305_DISPLAYOFF 0xAE
#define SSD1305_DISPLAYON 0xAF

#define SSD1305_SETPAGESTART 0xB0

#define SSD1305_COMSCANINC 0xC0
#define SSD1305_COMSCANDEC 0xC8
#define SSD1305_SETDISPLAYOFFSET 0xD3
#define SSD1305_SETDISPLAYCLOCKDIV 0xD5
#define SSD1305_SETAREACOLOR 0xD8
#define SSD1305_SETPRECHARGE 0xD9
#define SSD1305_SETCOMPINS 0xDA
#define SSD1305_SETVCOMLEVEL 0xDB

#define ADDR_I2C  0x78        // 0111 100x
//#define ADDR_I2C  0x3c        // 0011 1100

static void OLED_1305_WriteCmd(uint8_t cmd)
{
  static uint8_t buf[2] =
  {
      0x00,                             // Co = 0, C/D = 0
      0x00                              // place for value
  };

  buf[1] = cmd;
  I2C1_WriteBytes(ADDR_I2C, buf, 2);     // write
}

static void OLED_1305_SetPage(int pg)
{
  /*
  uint8_t buf[4] = { 0x00, SSD1305_SETPAGEADDR, 0x00, 0x00 };  // (Co = 0, C/D = 0), cmd, pg % 4, pg % 4
  buf[2] = (pg % 8);
  buf[3] = (pg % 8);

  I2C1_WriteBytes(ADDR_I2C, buf, 4);     // write
  */
  OLED_1305_WriteCmd(SSD1305_SETPAGEADDR);
  OLED_1305_WriteCmd(pg % 8);
  OLED_1305_WriteCmd(pg % 8);
}

static void OLED_1305_SetAllRow(void)
{
  /*
  uint8_t buf[4] = { 0x00, SSD1305_SETCOLADDR, 0x00, 0x00 };   // (Co = 0, C/D = 0), cmd, 0, OLED_WIDTH_BYTES
  buf[2] = 0;
  buf[3] = 128;

  I2C1_WriteBytes(ADDR_I2C, buf, 4);     // write
  */
  OLED_1305_WriteCmd(SSD1305_SETCOLADDR);
  OLED_1305_WriteCmd(0);
  OLED_1305_WriteCmd(128 - 1);
}

static uint8_t _imageBuffer[128 * 64 / 8];      //TODO from define, TODO from constructor ??
//TODO for DMA ... static uint8_t _transferBuffer[(128 + 1) * 64 / 8];

static void OLED_1305_DrawPixel(uint8_t x, uint8_t y, uint32_t color)
{
  if ((x < SSD1305_LCDWIDTH) && (y < SSD1305_LCDHEIGHT))
  {
    uint8_t *bufPtr = _imageBuffer + (y / 8) * SSD1305_LCDWIDTH + x;
    if (color)                      // zero or non-zero represents monochrome "colors"
      *bufPtr |= 1 << (y & 0x07);   // snad rychlejsi nez y % 8
    else
      *bufPtr &= ~(1 << (y & 0x07));
  }
}

//TODO variant for SPI connection
bool OLED_1305_Init(void)
{
  {
    uint32_t *uPtr = (uint32_t *)_imageBuffer;
    for(int i = 0; i < sizeof(_imageBuffer) / 4; i++)
      *uPtr++ = 0;
  }

  InitI2C1(i2cSpeed100k);

  // WaitMs(50);
  for(int i = 0; i < 10000; i++)
    __asm("NOP");                      // conditionally for ARM/KEIL and GCC compilers

  OLED_1305_WriteCmd(SSD1305_DISPLAYOFF);                    // 0xAE
  OLED_1305_WriteCmd(SSD1305_SETDISPLAYCLOCKDIV);            // 0xD5
  OLED_1305_WriteCmd(0x80);                                  // the suggested ratio 0x80

  OLED_1305_WriteCmd(SSD1305_SETMULTIPLEX);                  // 0xA8
  OLED_1305_WriteCmd(SSD1305_LCDHEIGHT - 1);                 // base on rows

  OLED_1305_WriteCmd(SSD1305_SETDISPLAYOFFSET);              // 0xD3
  OLED_1305_WriteCmd(0x0);                                   // no offset
  OLED_1305_WriteCmd(0);            // line #0
  OLED_1305_WriteCmd(0x8d);                    // 0x8D
  OLED_1305_WriteCmd(0x14);

  OLED_1305_WriteCmd(SSD1305_MEMORYMODE);                    // 0x20
  OLED_1305_WriteCmd(0x00);                                  // 0x0 act like ks0108, 00b, Horizontal Addressing Mode
  OLED_1305_WriteCmd(SSD1305_SEGREMAP | 0x1);                // 1 = column address 131 is mapped to SEG0
  OLED_1305_WriteCmd(SSD1305_COMSCANDEC);

  OLED_1305_WriteCmd(SSD1305_SETCOMPINS);                    // 0xDA
  OLED_1305_WriteCmd(0x12);
  OLED_1305_WriteCmd(SSD1305_SETCONTRAST);                   // 0x81
  OLED_1305_WriteCmd(0xCF);                                  // reset = 0x80

  OLED_1305_WriteCmd(SSD1305_SETPRECHARGE);                  // 0xd9
  OLED_1305_WriteCmd(0xF1);

  OLED_1305_WriteCmd(0xd8);                 // 0xD8
  OLED_1305_WriteCmd(0x40);
  OLED_1305_WriteCmd(SSD1305_DISPLAYALLON_RESUME);           // 0xA4
  OLED_1305_WriteCmd(SSD1305_NORMALDISPLAY);                 // 0xA6

//  OLED_1305_WriteCmd(SSD1306_DEACTIVATE_SCROLL);

  OLED_1305_WriteCmd(SSD1305_DISPLAYON);          //--turn on oled panel

  // WaitMs(50);
  for(int i = 0; i < 50000; i++)
    __asm("NOP");
/*
  static uint8_t lineBuf[128 + 1];
  lineBuf[0] = 0x40;                // Co = 1, C/D = 1
  for (int i = 0; i < 128; i++)
  {
    lineBuf[i + 1] = i;  // ~((uint8_t)i);
  }

  OLED_1305_SetPage(1);
  OLED_1305_SetAllRow();
  bool bb = I2C1_WriteBytes(ADDR_I2C, lineBuf, 128 + 1);

  for (int i = 0; i < 128; i++)
  {
    lineBuf[i + 1] = (i & 0x01) ? 0xaa : 0x55;
  }

  OLED_1305_SetPage(2);
  OLED_1305_SetAllRow();
  bb = I2C1_WriteBytes(ADDR_I2C, lineBuf, 128 + 1);

  return bb;
*/
  SetHiLevelDisp(SSD1305_LCDWIDTH, SSD1305_LCDHEIGHT, OLED_1305_DrawPixel);

  return true;
}

bool OLED_1305_UpdateContent(void)
{
  bool bbResult = true;
  for(int i = 0; i < 8; i++)                // 8 pages ... 64 pix height
  {
    OLED_1305_SetPage(i);
    OLED_1305_SetAllRow();
    bbResult = bbResult && I2C1_WriteBytesPre(ADDR_I2C, 0x40, _imageBuffer + i * 128, 128 + 1);
  }

  return bbResult;
}
