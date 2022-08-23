#include "stm_spi.h"

static STM_SpiDevice *pLockedDevice = NULL;

eSTM_SpiStates LastState = SPI_STATE_OK;            // is public

static bool STM_SPI_CR1_Init(STM_SpiDevice *pDevice, uint32_t busClock)
{ //! device->spi not tested, call internally only when is valid
  pDevice->spi->CR1 = 0
    | SPI_CR1_SSI | SPI_CR1_SSM
    | SPI_CR1_MSTR;

  if (pDevice->cpha) pDevice->spi->CR1 |= SPI_CR1_CPHA;
  if (pDevice->cpol) pDevice->spi->CR1 |= SPI_CR1_CPOL;
  if (pDevice->spi16b) pDevice->spi->CR1 |= SPI_CR1_DFF;

  if (pDevice->curSpeed == 0)      // not set ?
  {
    uint32_t BRDiv = 0;                 // 000 = pclk / 2
    uint32_t dx = busClock / pDevice->reqSpeed;

    //TODO do it as for cycle
    if (dx > 1) BRDiv = 0;   // 000 = pclk / 2
    if (dx > 2) BRDiv = 1;   // 001 = pclk / 4
    if (dx > 4) BRDiv = 2;   // 010 = pclk / 8
    if (dx > 8) BRDiv = 3;   // 011 = pclk / 16
    if (dx > 16) BRDiv = 4;   // 100 = pclk / 32
    if (dx > 32) BRDiv = 5;   // 101 = pclk / 64
    if (dx > 64) BRDiv = 6;   // 110 = pclk / 128
    // ...

    pDevice->curSpeed = busClock / (1 << (BRDiv + 1));
  }

  uint32_t xbrr = 1;
  for (uint32_t xdiv = busClock / pDevice->curSpeed; xdiv > 0; xdiv >>= 1)
    xbrr++;
  xbrr--;               // start with /2

  pDevice->spi->CR1 &= ~SPI_CR1_BR;
#if defined(STM32F4)                   //  only for F4xx
  pDevice->spi->CR1 |= (xbrr & 0x07) << SPI_CR1_BR_Pos;    // isolate 3 bits and set to bits position (here 5..3)
#else
#error Invalid platform type
#endif

  return true;
}

typedef struct _AF_Define_struct
{
  int spiNum;
  uint32_t xgpio;
  int pin;
  int af;
} AF_Define;

static AF_Define STM_SPI_AF_Defines[] =
{
    { 1, (uint32_t)GPIOA, 5, 5 },        // SPI1 - PA5 - AF5 - SCLK
    { 1, (uint32_t)GPIOA, 6, 5 },        // SPI1 - PA6 - AF5 - MISO
    { 1, (uint32_t)GPIOA, 7, 5 },        // SPI1 - PA7 - AF5 - MOSI
};

static uint32_t STM_SPI_GetAF(int spiNum, GPIO_TypeDef *port, uint32_t pin)
{
  for(int i = 0; i < sizeof(STM_SPI_AF_Defines) / sizeof(AF_Define); i++)
  {
    if ((STM_SPI_AF_Defines[i].spiNum == spiNum)
      && (STM_SPI_AF_Defines[i].xgpio == (uint32_t)port)
      && (STM_SPI_AF_Defines[i].pin == pin))
      return STM_SPI_AF_Defines[i].af;
  }

  return 0;       // indicates error
}

bool STM_SPI_LockDevice(STM_SpiDevice *pDevice)
{
  LastState = SPI_STATE_OK;

  if (pDevice == NULL)
  {
    LastState = SPI_STATE_NULL_Parameter;
    return false;
  }

  if (pLockedDevice == pDevice)
    return true;                      // not needed

  //TODO disable SPI INT, disable DMA, ...

  int spiNum = 0;

  switch((uint32_t)pDevice->spi)
  {
    case (uint32_t)SPI1:
      spiNum = 1;
      if (!(RCC->APB2ENR & RCC_APB2ENR_SPI1EN))
      {
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
        RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
      }

      SPI1->CR1 = 0;        //? clear settings, maybe clear only SPE bit
      STM_SPI_CR1_Init(pDevice, STM_GetBusClock(busClockAPB2));     //TODO check success

      SPI1->CR2 = 0;        //TODO set for DMA

      SPI1->CR1 |= SPI_CR1_SPE;

      STM_SetPinGPIO(pDevice->SCLKPort, pDevice->SCLKPin, ioPortAlternatePP);
      STM_SetAFGPIO(pDevice->SCLKPort, pDevice->SCLKPin,
          STM_SPI_GetAF(spiNum, pDevice->SCLKPort, pDevice->SCLKPin));

      if (pDevice->MISOPort != NULL)
      {
        STM_SetPinGPIO(pDevice->MISOPort, pDevice->MISOPin, ioPortAlternatePP);
        STM_SetAFGPIO(pDevice->MISOPort, pDevice->MISOPin,
            STM_SPI_GetAF(spiNum, pDevice->MISOPort, pDevice->MISOPin));
      }

      STM_SetPinGPIO(pDevice->MOSIPort, pDevice->MOSIPin, ioPortAlternatePP);
      STM_SetAFGPIO(pDevice->MOSIPort, pDevice->MOSIPin,
          STM_SPI_GetAF(spiNum, pDevice->MOSIPort, pDevice->MOSIPin));

      STM_SetPinGPIO(pDevice->CSPort, pDevice->CSPin, ioPortOutputPP);
      GPIOWrite(pDevice->CSPort, pDevice->CSPin, 1);
      break;
    default:
      LastState = SPI_STATE_NULL_Parameter;
      return false;
  }

  return true;
}

bool STM_SPI_UnLockDevice(STM_SpiDevice *pDevice)
{
  if (pLockedDevice != pDevice)
    return false;

  GPIOWrite(pDevice->CSPort, pDevice->CSPin, 1);                // set CS = 1
  pDevice->spi->CR1 &= ~SPI_CR1_SPE;                            // stop SPI working

  pLockedDevice = NULL;
  return true;
}

bool STM_SPI_IsLocked(void)
{
  return pLockedDevice != NULL;
}
