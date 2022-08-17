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

/*
static void OLED_1305_WriteCmd8(uint8_t adr, uint8_t value)
{
  I2C_Start();
  I2C_Addr(ADDR_I2C);                   // write
  I2C_Write(0x00);                      // Co = 0, C/D = 0
  I2C_Write(adr);
  I2C_Write(value);
  I2C_Stop();
}

static void OLED_1305_WriteCmd16(uint8_t adr, uint16_t value)
{
  I2C_Start();
  I2C_Addr(ADDR_I2C);                   // write
  I2C_Write(0x00);                      // Co = 0, C/D = 0
  I2C_Write(adr);
  I2C_Write(value / 256);
  I2C_Write(value % 256);
  I2C_Stop();
}

static void OLED_1305_WriteData(uint8_t value)
{
  I2C_Start();
  I2C_Addr(ADDR_I2C);                   // write
  I2C_Write(0x40);                      // Co = 1, C/D = 1
  I2C_Write(value);
  I2C_Stop();
}

static void OLED_1305_WriteDataBlock(uint8_t *pValues, int len)
{
  I2C_Start();
  I2C_Addr(ADDR_I2C);                   // write
  I2C_Write(0x40);                      // Co = 1, C/D = 1
  while(len--)
  {
    I2C_Write(*pValues);
    pValues++;
  }
  I2C_Stop();
}
*/

static void OLED_1305_SetPage(int pg)
{
//  MiniOLED_WriteCmd16(0x22, ((pg % 8) << 8) + (pg % 8));   // SSD1305_SETPAGEADDR, pg % 4, pg % 4
  OLED_1305_WriteCmd(0x22);
  OLED_1305_WriteCmd(pg % 8);
  OLED_1305_WriteCmd(pg % 8);
}

static void OLED_1305_SetAllRow(void)
{
//  MiniOLED_WriteCmd16(0x21, ((0) << 8) + (128));   // SSD1305_SETCOLADDR, 0, OLED_WIDTH_BYTES
  OLED_1305_WriteCmd(0x21);
  OLED_1305_WriteCmd(0);
  OLED_1305_WriteCmd(128 - 1);
}

//TODO variant for SPI connection
bool OLED_1305_Init(void)
{
  InitI2C1(i2cSpeed100k);

  // WaitMs(50);
  for(int i = 0; i < 10000; i++)
    __asm("NOP");                      // conditionally for ARM/KEIL and GCC compilers

  OLED_1305_WriteCmd(0xAE);                    // 0xAE
  OLED_1305_WriteCmd(0xd5);            // 0xD5
  OLED_1305_WriteCmd(0x80);                                  // the suggested ratio 0x80

  OLED_1305_WriteCmd(0xa8);                  // 0xA8
  OLED_1305_WriteCmd(64 - 1);

  OLED_1305_WriteCmd(0xd3);              // 0xD3
  OLED_1305_WriteCmd(0x0);                                   // no offset
  OLED_1305_WriteCmd(0);            // line #0
  OLED_1305_WriteCmd(0x8d);                    // 0x8D
  OLED_1305_WriteCmd(0x14);

  OLED_1305_WriteCmd(0x20);                    // 0x20
  OLED_1305_WriteCmd(0x00);                                  // 0x0 act like ks0108
  OLED_1305_WriteCmd(0xa0 | 0x1);
  OLED_1305_WriteCmd(0xc8);

  OLED_1305_WriteCmd(0xda);                    // 0xDA
  OLED_1305_WriteCmd(0x12);
  OLED_1305_WriteCmd(0x81);                   // 0x81
  OLED_1305_WriteCmd(0xCF);

  OLED_1305_WriteCmd(0xd9);                  // 0xd9
  OLED_1305_WriteCmd(0xF1);

  OLED_1305_WriteCmd(0xd8);                 // 0xDB
  OLED_1305_WriteCmd(0x40);
  OLED_1305_WriteCmd(0xa4);           // 0xA4
  OLED_1305_WriteCmd(0xa6);                 // 0xA6

//  MiniOLED_WriteCmd(SSD1306_DEACTIVATE_SCROLL);

  OLED_1305_WriteCmd(0xaf);          //--turn on oled panel

  // WaitMs(50);
  for(int i = 0; i < 10000; i++)
    __asm("NOP");

  uint8_t lineBuf[128 + 1];
  lineBuf[0] = 0x40;                // Co = 1, C/D = 1
  for (int i = 0; i < 128; i++)
  {
    lineBuf[i + 1] = i;
  }

  OLED_1305_SetPage(1);
  OLED_1305_SetAllRow();
  bool bb = I2C1_WriteBytes(ADDR_I2C, lineBuf, 128 + 1);

  return true;
}
