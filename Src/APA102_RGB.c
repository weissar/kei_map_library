#include "APA102_RGB.h"

#define PIN_APA_102_CLK GPIOB,3               // PB3 = SPI3 CLK
#define PIN_APA_102_DAT GPIOB,5               // PB5 = SPI3 MOSI

#define APA_102_SPI

// asi zatim ne, musel bych mit buffer pro cely paket a nakopirovat tam data + zacatek a konec...
// #define APA_102_SPI_DMA

void RGB_Rainbow_32_Generate(RGB_LED *led_array)    // count not set, fix size = 32 elements
{
  // https://krazydad.com/tutorials/makecolors.php
  float frequency = .3;
  for (int i = 0; i < RAINBOW_COLORS_COUNT; ++i)
  {
    led_array[i].r = sin(frequency*i + 0) * 127 + 128;
    led_array[i].g = sin(frequency*i + 2) * 127 + 128;
    led_array[i].b = sin(frequency*i + 4) * 127 + 128;
  }
}

void RGB_Set_SetIntesity(RGB_LED *led_array, int count, uint8_t val)
{
  val &= 0x1f;                      // only lower 5 bits is valid
  for(int i = 0; i < count; i++)
    led_array[i].alpha = 0xE0 | val;
}

#ifdef APA_102_SPI
bool APA_102_init(void)
{
  if (!(RCC->APB1ENR & RCC_APB1ENR_SPI3EN))
  {
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    RCC->APB1RSTR |= RCC_APB1RSTR_SPI3RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_SPI3RST;
  }

  SPI3->CR1 = SPI_CR1_BR_1;       // 010 = clk/8 - from APB1 (max. 50MHz), eg. 2M for 16M
  SPI3->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM;    // work as master, 8-bit mode (DFF = 0)
  SPI3->CR1 |= 0;                 // CPHA = 0, CPOL = 0 - see RM0383 pg. 555/836 (rev 1)
  SPI3->CR2 = 0;

  SPI3->CR1 |= SPI_CR1_SPE;

  STM_SetPinGPIO(PIN_APA_102_CLK, ioPortAlternatePP);
  STM_SetAFGPIO(PIN_APA_102_CLK, 6);
  STM_SetPinGPIO(PIN_APA_102_DAT, ioPortAlternatePP);
  STM_SetAFGPIO(PIN_APA_102_DAT, 6);

  return true;
}

void SPISend32(uint32_t val)
{
  uint8_t *dataPtr = (uint8_t *)&val;
  for (int i = 0; i < sizeof(uint32_t); i++)
  {
    while (!(SPI3->SR & SPI_SR_TXE) || (SPI3->SR & SPI_SR_BSY))
      ;

    SPI3->DR = *dataPtr;
    dataPtr++;
  }
}

void APA_102_SendRGB(RGB_LED *ledArray, int lenArray, int offset, int countLED)
{
  SPISend32(0x00000000);              // start packet

  for(int i = 0; i < 4; i++)
  {
    SPISend32(*(uint32_t *)(ledArray + offset));        // LittleEndian !!

    offset++;
    if (offset >= lenArray)
      offset = 0;
  }

  SPISend32(~0x00000000);           // end packet
}

#else
bool APA_102_init(void)
{
  STM_SetPinGPIO(PIN_APA_102_CLK, ioPortOutputPP);
  STM_SetPinGPIO(PIN_APA_102_DAT, ioPortOutputPP);
  return true;
}

void ShiftSend8(uint8_t val)
{
  for(int i = 0; i < 8; i++)
  {
    GPIOWrite(PIN_APA_102_DAT, val & 0x80);

    GPIOWrite(PIN_APA_102_CLK, 1);
    GPIOWrite(PIN_APA_102_CLK, 0);

    val <<= 1;
  }
}

void ShiftSend32(uint32_t val)
{
  for(int i = 0; i < 32; i++)
  {
    GPIOWrite(PIN_APA_102_DAT, val & (1 << 31));

    GPIOWrite(PIN_APA_102_CLK, 1);
    GPIOWrite(PIN_APA_102_CLK, 0);

    val <<= 1;
  }
}

void Send_APA102_LED(uint8_t r, uint8_t g, uint8_t b, uint8_t glob)
{
  glob |= 0xE0;         // 111x xxxx

  ShiftSend8(glob);
  ShiftSend8(b);
  ShiftSend8(g);
  ShiftSend8(r);
}

void APA_102_SendRGB(RGB_LED *ledArray, int lenArray, int offset, int countLED)
{
  ShiftSend32(0x00000000);              // start packet

  for(int i = 0; i < 4; i++)
  {
    Send_APA102_LED(ledArray[offset].r, ledArray[offset].g, ledArray[offset].b, ledArray[offset].alpha);

    offset++;
    if (offset >= lenArray)
      offset = 0;
  }

  ShiftSend32(~0x00000000);           // end packet
}
#endif

