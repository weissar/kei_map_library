#ifndef _STM_SPI_H
#define _STM_SPI_H

#include "stm_core.h"

typedef struct _STM_SpiDevice
{
  SPI_TypeDef *spi;
  uint32_t reqSpeed;
  uint32_t curSpeed;
  bool spi16b;
  bool cpha;
  bool cpol;
  GPIO_TypeDef  *SCLKPort;
  uint32_t SCLKPin;
  GPIO_TypeDef  *MOSIPort;
  uint32_t MOSIPin;
  GPIO_TypeDef  *MISOPort;
  uint32_t MISOPin;
  GPIO_TypeDef  *CSPort;
  uint32_t CSPin;
} STM_SpiDevice;

typedef enum
{
  SPI_STATE_OK,
  SPI_STATE_NULL_Parameter,
  SPI_STATE_Unknown_SPI,
}  eSTM_SpiStates;

#ifndef NULL
#define NULL  ((void *)0L)
#endif

extern eSTM_SpiStates LastState;            // is public

bool STM_SPI_LockDevice(STM_SpiDevice *pDevice);
bool STM_SPI_UnLockDevice(STM_SpiDevice *pDevice);
bool STM_SPI_IsLocked(void);

#endif  // _STM_SPI_H
