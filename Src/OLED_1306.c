#include "OLED_1306.h"
#include "stm_spi.h"

#define OLED_1306_WIDTH_PIX       128
#define OLED_1306_HEIGHT_PIX      64
#define OLED_1306_WIDTH_BYTES     128  // 132
#define OLED_1306_HEIGHT_BYTES    8

static void OLED_1306_Write8(uint8_t b);
static void OLED_1306_WriteCmd(uint8_t cmd);
static void OLED_1306_SetPage(int pg);
static void OLED_1306_SetAllRow(void);

static bool OLED_1306_Init_HW(void);
static void OLED_1306_Init_SW(void);

#define OLED_1306_MOSI  NUCLEO_D11
#define OLED_1306_SCK   NUCLEO_D13
#define OLED_1306_RST   NUCLEO_A5
#define OLED_1306_CSN   NUCLEO_D6
#define OLED_1306_DC    NUCLEO_D9

#ifndef SPI_WAIT
#define SPI_IS_BUSY(SPIx) (((SPIx)->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || ((SPIx)->SR & SPI_SR_BSY))
#define SPI_WAIT(SPIx)            while (SPI_IS_BUSY(SPIx))
#endif

#ifdef _STM_SPI_H
static STM_SpiDevice deviceSPI =
{
  .reqSpeed = 15E6,                         // teoretically 10MHz, try faster ...
  .curSpeed = 0,                            // will be calculated
  .cpha = true, .cpol = true,               // see RM0383 pg. 555/836 (rev 1)
  .CSPort = GPIOB, .CSPin = 10,             // PB10 - Arduino D6
  .SCLKPort = GPIOA, .SCLKPin = 5,
  .MISOPort = NULL,
  .MOSIPort = GPIOA, .MOSIPin = 7,
  .spi16b = false,
  .spi = SPI1
};
static bool OLED_1306_Init_HW(void)
{
  STM_SPI_LockDevice(&deviceSPI);

  STM_SPI_UnLockDevice(&deviceSPI);         // free resource, but CS remain

  STM_SetPinGPIO(OLED_1306_RST, ioPortOutputPP);
  GPIOWrite(OLED_1306_RST, 1);
  STM_SetPinGPIO(OLED_1306_DC, ioPortOutputPP);
  GPIOWrite(OLED_1306_DC, 1);

  return true;
}
#else
static void OLED_1306_Init_HW(void)
{
  if (!(RCC->APB2ENR & RCC_APB2ENR_SPI1EN))
  {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
  }

  SPI1->CR1 = SPI_CR1_BR_1 | SPI_CR1_BR_0;      // 011 = clk/16 - z APB2 (100MHz), LCD max. 10MHz
  SPI1->CR1 |= SPI_CR1_MSTR;
  SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM;
  SPI1->CR1 |= SPI_CR1_CPHA | SPI_CR1_CPOL; // viz. RM0383 pg. 555/836 (rev 1)
  SPI1->CR2 = 0;

  SPI1->CR1 |= SPI_CR1_SPE;

  STM_SetPinGPIO(OLED_1306_SCK, ioPortAlternatePP);
  STM_SetAFGPIO(OLED_1306_SCK, 5);
  STM_SetPinGPIO(OLED_1306_MOSI, ioPortAlternatePP);
  STM_SetAFGPIO(OLED_1306_MOSI, 5);

  STM_SetPinGPIO(OLED_1306_CSN, ioPortOutputPP);
  GPIOWrite(OLED_1306_CSN, 1);
  STM_SetPinGPIO(OLED_1306_RST, ioPortOutputPP);
  GPIOWrite(OLED_1306_RST, 1);
  STM_SetPinGPIO(OLED_1306_DC, ioPortOutputPP);
  GPIOWrite(OLED_1306_DC, 1);
}
#endif

static volatile int xPtr = 0, yPtr = 0;
static volatile bool bbUseDMA = true;

static volatile uint8_t oledBuffer[OLED_1306_HEIGHT_BYTES][OLED_1306_WIDTH_BYTES];
static volatile uint8_t oledSendBuffer[OLED_1306_HEIGHT_BYTES][OLED_1306_WIDTH_BYTES];

static volatile bool refreshInProgess = false;

static void OLED_1306_Write8(uint8_t b)
{
  SPI1->DR = b;
//  while(!(SPI1->SR & SPI_SR_BSY))  // wait for finish
  SPI_WAIT(SPI1)
    ;
}

static void OLED_1306_WriteCmd(uint8_t cmd)
{
  GPIOWrite(OLED_1306_DC, 0);
  GPIOWrite(OLED_1306_CSN, 0);

  OLED_1306_Write8(cmd);

  GPIOWrite(OLED_1306_CSN, 1);
}

static void OLED_1306_Init_SW(void)
{
#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif
  uint32_t tm = 0;

  GPIOWrite(OLED_1306_RST, 0);
  for(tm = 0; tm < 50000; tm++)
    asm("nop");

  GPIOWrite(OLED_1306_RST, 1);
  for(tm = 0; tm < 10000; tm++)
    asm("nop");

  OLED_1306_WriteCmd(0xAE);                    // 0xAE
  OLED_1306_WriteCmd(0xd5);            // 0xD5
  OLED_1306_WriteCmd(0x80);                                  // the suggested ratio 0x80

  OLED_1306_WriteCmd(0xa8);                  // 0xA8
  OLED_1306_WriteCmd(64 - 1);

  OLED_1306_WriteCmd(0xd3);              // 0xD3
  OLED_1306_WriteCmd(0x0);                                   // no offset
  OLED_1306_WriteCmd(0);            // line #0
  OLED_1306_WriteCmd(0x8d);                    // 0x8D
  OLED_1306_WriteCmd(0x14);

  OLED_1306_WriteCmd(0x20);                    // 0x20
  OLED_1306_WriteCmd(0x00);                                  // 0x0 act like ks0108
  OLED_1306_WriteCmd(0xa0 | 0x1);
  OLED_1306_WriteCmd(0xc8);

  OLED_1306_WriteCmd(0xda);                    // 0xDA
  OLED_1306_WriteCmd(0x12);
  OLED_1306_WriteCmd(0x81);                   // 0x81
  OLED_1306_WriteCmd(0xCF);

  OLED_1306_WriteCmd(0xd9);                  // 0xd9
  OLED_1306_WriteCmd(0xF1);

  OLED_1306_WriteCmd(0xd8);                 // 0xDB
  OLED_1306_WriteCmd(0x40);
  OLED_1306_WriteCmd(0xa4);           // 0xA4
  OLED_1306_WriteCmd(0xa6);                 // 0xA6

//  OLED_WriteCmd(SSD1306_DEACTIVATE_SCROLL);

  OLED_1306_WriteCmd(0xaf);          //--turn on oled panel

  /* pro 1305 stacilo jen nasledujici
  OLED_DC_LOW;
  OLED_CS_LOW;

  OLED_Write8(SSD1305_MEMORYMODE);
  OLED_Write8(0x00);

  OLED_CS_HIGH;

  OLED_WriteCmd(SSD1305_DISPLAYON);
  */
#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
}

static void OLED_1306_SetPage(int pg)
{
  OLED_1306_WriteCmd(0x22);
  OLED_1306_WriteCmd(pg %  OLED_1306_HEIGHT_BYTES);
  OLED_1306_WriteCmd(pg % OLED_1306_HEIGHT_BYTES);

  /* mozna by stacilo to udelat na jeden CS
  OLED_DC_LOW;
  OLED_CS_LOW;

  OLED_Write8(SSD1305_SETPAGEADDR);
  OLED_Write8(pg % 4);
  OLED_Write8(pg % 4);

  OLED_CS_HIGH;
  */
}

static void OLED_1306_SetAllRow(void)
{
  OLED_1306_WriteCmd(0x21);
  OLED_1306_WriteCmd(0);
  OLED_1306_WriteCmd(128 - 1);

  /* mozna jednim vrzem lepe
  OLED_DC_LOW;
  OLED_CS_LOW;

  OLED_Write8(SSD1305_SETCOLADDR);
  OLED_Write8(0);
  OLED_Write8(OLED_WIDTH_BYTES);

  OLED_CS_HIGH;
  */
}

bool OLED_1306_Refresh(void)
{
  if (refreshInProgess)
  {
    //TODO show error
    return false;
  }

  refreshInProgess = true;

  #ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif
  uint y, x;

  for(y = 0; y < OLED_1306_HEIGHT_BYTES; y++)
    for(x = 0; x < OLED_1306_WIDTH_BYTES; x++)
      oledSendBuffer[y][x] = oledBuffer[y][x];

  for(y = 0; y < OLED_1306_HEIGHT_BYTES; y++)
  {
    OLED_1306_SetPage(y);
    OLED_1306_SetAllRow();

    GPIOWrite(OLED_1306_DC, 1);
    GPIOWrite(OLED_1306_CSN, 0);

    for(x = 0; x < OLED_1306_WIDTH_BYTES; x++)
      OLED_1306_Write8(oledSendBuffer[y][x]);

    GPIOWrite(OLED_1306_CSN, 1);
  }
#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif

  refreshInProgess = false;

  return true;
}

static void OLED_1306_DrawPixel(int x, int y, uint32_t color)
{
  if ((x < OLED_1306_WIDTH_PIX) && (y < OLED_1306_HEIGHT_PIX))
  {
    if (color)
      oledBuffer[y / 8][x] |= 1 << (y & 0x07);   // snad rychlejsi nez y % 8
    else
      oledBuffer[y / 8][x] &= ~(1 << (y & 0x07));
  }
}

static void OLED_1306_SetByte(int x, int row, uint8_t val)
{
  oledBuffer[row][x] = val;
}

// nechce se mi to cele includovat ... #include "disp_hilevel.h"
void SetHiLevelDisp(int pixWidth, int pixHeight, void *ptrDrawPixel);

bool OLED_1306_Init(void)
{
  bool bbRet = true;
  OLED_1306_Init_HW();
  OLED_1306_Init_SW();

  SetHiLevelDisp(OLED_1306_WIDTH_PIX, OLED_1306_HEIGHT_PIX, OLED_1306_DrawPixel);

  return bbRet;
}
