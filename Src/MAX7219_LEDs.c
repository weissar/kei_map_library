#include "MAX7219_LEDs.h"
#include "stm_spi.h"

#ifndef SPI_WAIT
#define SPI_IS_BUSY(SPIx) (((SPIx)->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || ((SPIx)->SR & SPI_SR_BSY))
#define SPI_WAIT(SPIx)            while (SPI_IS_BUSY(SPIx))
#endif

#define USE_SPI_16B

#ifdef _STM_SPI_H
static STM_SpiDevice deviceSPI =
{
  .reqSpeed = 10E6,                         // MAX7219 max. 10MHz
  .curSpeed = 0,                            // will be calculated
  .cpha = true, .cpol = true,               // see RM0383 pg. 555/836 (rev 1)
//  .CSPort = GPIOB, .CSPin = 6,
  .CSPort = GPIOA, .CSPin = 6,
  .SCLKPort = GPIOA, .SCLKPin = 5,
  .MISOPort = NULL,
  .MOSIPort = GPIOA, .MOSIPin = 7,
#ifdef USE_SPI_16B
  .spi16b = true,
#else
  .spi16b = false,
#endif
  .spi = SPI1
};
static bool _MAX7219_InitHW(void)
{
  STM_SPI_LockDevice(&deviceSPI);

  STM_SPI_UnLockDevice(&deviceSPI);         // free resource, but CS remain
  return true;
}
#else
static bool _MAX7219_InitHW(void)
{
  if (!(RCC->APB2ENR & RCC_APB2ENR_SPI1EN))
  {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
  }

  SPI1->CR1 = SPI_CR1_BR_1 | SPI_CR1_BR_0; // 011 = clk/16 - from APB2 (100MHz), MAX7219 max. 10MHz ?
  SPI1->CR1 |= SPI_CR1_MSTR;
#ifdef USE_SPI_16B
  SPI1->CR1 |= SPI_CR1_DFF;
#endif
  SPI1->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM;
  SPI1->CR1 |= SPI_CR1_CPHA | SPI_CR1_CPOL; // see RM0383 pg. 555/836 (rev 1)
  SPI1->CR2 = 0;

  SPI1->CR1 |= SPI_CR1_SPE;

  STM_SetPinGPIO(GPIOA, 5, ioPortAlternatePP);   // D13 - SPI1 - SCK,  SCK
  STM_SetAFGPIO(GPIOA, 5, 5);
//  Nucleo_SetPinGPIO(GPIOA, 6, alter);   // D12 - SPI1 - MISO, SDO
//  Nucleo_SetPinAFGPIO(GPIOA, 6, 5);
  STM_SetPinGPIO(GPIOA, 7, ioPortAlternatePP);   // D11 - SPI1 - MOSI, MOSI
  STM_SetAFGPIO(GPIOA, 7, 5);

  STM_SetPinGPIO(GPIOB, 6, ioPortOutputPP);      // D10 = CS
  GPIOWrite(GPIOB, 6, 1);

  return true;
}
#endif

static void _MAX7219_SendWord(uint16_t w)
{
  SPI_WAIT(deviceSPI.spi);
  GPIOWrite(deviceSPI.CSPort, deviceSPI.CSPin, 0);

#ifdef USE_SPI_16B
  SPI1->DR = w;           // complete 16b value
  SPI_WAIT(SPI1);
#else
#error Pozor, SPI a CS natvrdo
  SPI1->DR = (w >> 8);
  while (!(SPI1->SR & SPI_SR_TXE))
    ;                     //! not needed wait to complete, enough is "transmit empty"
  SPI1->DR = w & 0xff;
  SPI_WAIT(SPI1);
#endif
  GPIOWrite(deviceSPI.CSPort, deviceSPI.CSPin, 1);
}

// 7seg decoder
//   6
// 1   5
//   0
// 2   4
//   3
//       7

static const uint8_t to7seg[16] =
{
  0b01111110,
  0b00110000,
  0b01101101,
  0b01111001,
  0b00110011,
  0b01011011,
  0b01011111,
  0b01110000,
  0b01111111,
  0b01111011,
  0b01110111,
  0b00011111,
  0b01001110,
  0b00111101,
  0b01001111,
  0b01000111
};

static bool _MAX7219_InitSW(int count)
{
#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#else
  if ((SPI1->CR1 & SPI_CR1_SPE) == 0)
    return false;
#endif
                          // X0xx = NoOP
                          // X1xx - X9xx = Digit 0 .. 7, MSB - DP A B C D E F G
  _MAX7219_SendWord(0x0f00);    // XFxx = display test, xx00 = normal, xx01 = test
  _MAX7219_SendWord(0x0b00 | (count % 8));    // XBxx = scan limit, xxXb = 0 = digit 0 .. 7 = digit 0,1,2..7
  _MAX7219_SendWord(0x0900);    // X9xx = decode-mode, xx00 = no decode
  _MAX7219_SendWord(0x0c01);    // XCxx = shutdown register, xx01 = normal operation
  _MAX7219_SendWord(0x0aff);    // XAxx = intensity, xxXa = a = 1 .. 31 / 32 (step 2)

//  _MAX7219_SendWord(0x0f01);
//  WaitMs(1000);

//  _MAX7219_SendWord(0x0f00);

#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
return true;
}

uint8_t MAX7219_DecodeSeg(uint8_t val)
{
  return to7seg[val % 16];
}

uint8_t MAX7219_AddPoint(uint8_t val)
{
  return val | 0x80;                      // segment P
}

bool MAX7219_ViewHEX(uint16_t x)
{
  uint8_t b = 4;

#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif

  _MAX7219_SendWord((b << 8) + to7seg[x % 16]);
  x >>= 4; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 16]);
  x >>= 4; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 16]);
  x >>= 4; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 16]);

#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
  return true;
}

bool MAX7219_ViewDEC(uint16_t x)
{
  uint8_t b = 4;
  bool over = x > 9999;
  x %= 10000;

#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif

  _MAX7219_SendWord((b << 8) + to7seg[x % 10]);
  x /= 10; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 10]);
  x /= 10; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 10]);
  x /= 10; b--;
  _MAX7219_SendWord((b << 8) + to7seg[x % 10]);

#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
  return over;
}

bool MAX7219_SetIntensity(uint8_t b)
{
#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif

  _MAX7219_SendWord(0x0a00 | b);
#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
  return true;
}

bool MAX7219_BitContent(uint8_t *pData)
{
#ifdef _STM_SPI_H
  STM_SPI_LockDevice(&deviceSPI);
#endif

  //  for (uint8_t b = 4; b >= 1; b--)
  for (uint8_t b = 1; b <= 4; b++)
  {
    _MAX7219_SendWord((b << 8) + *pData);
    pData++;
  }

#ifdef _STM_SPI_H
  STM_SPI_UnLockDevice(&deviceSPI);
#endif
  return true;
}

bool MAX7219_Init(void)
{
  if (_MAX7219_InitHW() && _MAX7219_InitSW(4))
  {
    MAX7219_ViewHEX(0);       // 0000
    return true;
  }
  else
    return false;
}
