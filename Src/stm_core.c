/*
 * stm_core.c
 *
 *  Created on: 6. 2. 2017
 *      Author: Petr
 */

#include "stm_core.h"

bool STM_SetPinGPIO(GPIO_TypeDef *gpio, uint32_t bitnum, eIoPortModes mode)
{
  uint32_t enr_mask = 0;     // value for xxENR register
  uint32_t rstr_mask = 0;    // value for xxRSTR register

  // detekce, ktery GPIO
  switch((uint32_t)gpio)
  {
    case (uint32_t)GPIOA:
        enr_mask = RCC_AHB1ENR_GPIOAEN;
        rstr_mask = RCC_AHB1RSTR_GPIOARST;
      break;
    case (uint32_t)GPIOB:
      enr_mask = RCC_AHB1ENR_GPIOBEN;
      rstr_mask = RCC_AHB1RSTR_GPIOBRST;
      break;
    case (uint32_t)GPIOC:
      enr_mask = RCC_AHB1ENR_GPIOCEN;
      rstr_mask = RCC_AHB1RSTR_GPIOCRST;
      break;
    case (uint32_t)GPIOD:
      enr_mask = RCC_AHB1ENR_GPIODEN;
      rstr_mask = RCC_AHB1RSTR_GPIODRST;
      break;
    case (uint32_t)GPIOE:
      enr_mask = RCC_AHB1ENR_GPIOEEN;
      rstr_mask = RCC_AHB1RSTR_GPIOERST;
      break;
#if defined(STM32F411xE)   // 411RE without GPIOF a G !!
#else
    case (uint32_t)GPIOF:
      enr_mask = RCC_AHB1ENR_GPIOFEN;
      rstr_mask = RCC_AHB1RSTR_GPIOFRST;
      break;
    case (uint32_t)GPIOG:
      enr_mask = RCC_AHB1ENR_GPIOGEN;
      rstr_mask = RCC_AHB1RSTR_GPIOGRST;
      break;
#endif
    case (uint32_t)GPIOH:
      enr_mask = RCC_AHB1ENR_GPIOHEN;
      rstr_mask = RCC_AHB1RSTR_GPIOHRST;
      break;
  }

  if ((enr_mask == 0) || (rstr_mask == 0))    // nothing selected GPIO
    return false;       // return error flag

  // init selected IO
  if (!(RCC->AHB1ENR & enr_mask))
  {
    RCC->AHB1ENR |= enr_mask;      // enable peripheral clock
    RCC->AHB1RSTR |= rstr_mask;    // do reset of peripheral
    RCC->AHB1RSTR &= ~rstr_mask;   // and finish of reset (pulse)
  }

  // setting config bits to default state (can be non-default/reset state)
  gpio->MODER &= ~(0x03 << (2 * bitnum));     // clear corresponding 2 bits
  gpio->PUPDR &= ~(0x03 << (2 * bitnum));     // clear corresponding 2 bits
  gpio->OSPEEDR &= ~(0x03 << (2 * bitnum));   // clear corresponding 2 bits

  // set registers by IO type
  switch(mode)
  {
    case ioPortOutputOC:
    case ioPortOutputPP:
      gpio->MODER |= 0x01 << (2 * bitnum);          // 01 = output
      gpio->OSPEEDR |= 0x03 << (2 * bitnum);        // 11 = high speed
      gpio->PUPDR &= ~(0x03 << (2 * bitnum));       // 00 = no pu/pd

      if (mode == ioPortOutputOC)                   // open collector ?
        gpio->OTYPER |= 0x01 << bitnum;             // yes - set 1 = OC/Open drain
      else
        gpio->OTYPER &= ~(0x01 << bitnum);          // no - set 0 = push-pull
      break;
    case ioPortInputPU:                             // moder bits 00 = input
      gpio->PUPDR &= ~(0x03 << (2 * bitnum));       // clear bits
      gpio->PUPDR |= 0x01 << (2 * bitnum);          // 01 = pull-up
      break;
    case ioPortInputPD:
      gpio->PUPDR &= ~(0x03 << (2 * bitnum));       // clear bits
      gpio->PUPDR |= 0x02 << (2 * bitnum);          // 10 = pull-down
      break;
    case ioPortInputFloat:                          // 00 = input mode, nothing to do
      gpio->PUPDR &= ~(0x00000003 << (2 * bitnum)); // 00 = no pull-up/dn
      break;
    case ioPortAlternatePP:
    case ioPortAlternateOC:
      gpio->MODER |= 0x00000002 << (2 * bitnum);    // set bits to 2 (binary 10)

      if (mode == ioPortAlternateOC)
      {
        gpio->OTYPER |= 0x00000001 << bitnum;       // 1 = open-drain, one bit per GPIO
        gpio->PUPDR &= ~(0x00000001 << (2 * bitnum)); // 01 = pull-up for OC
      }
      else
        gpio->OTYPER &= ~(0x00000001 << bitnum);    // 0 = push-pull, one bit per GPIO

      gpio->OSPEEDR |= 0x00000003 << (2 * bitnum);  // high-speed = 11

      //!! don't forget set AFR registers !!!
      break;
    case ioPortAnalog:                              // 11 - analog mode
      gpio->MODER |= 0x00000003 << (2 * bitnum);    // set bits
      break;
    default:                                        // unknown mode ?
      return false;                                 // return "error"
  }

  return true;                                      // return "OK"
}

bool STM_SetAFGPIO(GPIO_TypeDef *gpio, uint32_t bitnum, uint32_t afValue)
{
  gpio->AFR[(bitnum < 8) ? 0 : 1] &= ~(0x0f << (4 * (bitnum & 0x07)));   // clear AF bits
  gpio->AFR[(bitnum < 8) ? 0 : 1] |= ((afValue & 0x0f) << (4 * (bitnum & 0x07)));   // set AF bits
  return true;
}

void GPIOToggle(GPIO_TypeDef *gpio, uint32_t bitnum)
{
  gpio->ODR ^= 1 << bitnum;
}

bool GPIORead(GPIO_TypeDef *gpio, uint32_t bitnum)
{
  return ((gpio->IDR & (1 << bitnum)) != 0);    // compare = produce logic value
}

void GPIOWrite(GPIO_TypeDef *gpio, uint32_t bitnum, bool state)
{
  gpio->BSRR = (state) ? (1 << (bitnum)) : ((1 << (bitnum)) << 16);
        // BSRR register - lower 16 bits = Set to 1,  higher 16 bits = Reset to 0
}

uint32_t STM_GetTimerClock(int timerNum)
{
  uint32_t apbdiv = 0, timerClock = SystemCoreClock;

#if defined(STM32F411xE) // || defined(STM32F413xx)  // || defined ...
  switch(timerNum)
  {
    case 1:
    case 9:
    case 10:
    case 11:
      apbdiv = RCC->CFGR & RCC_CFGR_PPRE2;    // 0x0000E000 - keep bits 15..13
      apbdiv >>= 13;                          // move right
      break;
    case 2:
    case 3:
    case 4:
    case 5:
      apbdiv = RCC->CFGR & RCC_CFGR_PPRE1;    // 0x00001C00 - keep bits 12:10
      apbdiv >>= 10;
      break;
    default:
      //TODO emit error for unknown Timer
      break;
  }
#else
#error Valid controller not set - GetTimerClock
#endif

  if ((apbdiv & 0x04) == 0)                   // MSB of that 3 bits is 0 ?
    timerClock = SystemCoreClock;             // not divided, eg. x1
  else
    timerClock = 2 * (SystemCoreClock >> ((apbdiv & 0x03) + 1));   // lower 2 bits is divider

  return timerClock;
}

uint32_t STM_GetBusClock(eBusClocks clk)
{
  uint32_t bitval = 0;
  uint32_t divider = 1;

#if defined(STM32F411xE) || defined(STM32F413xx)  // || defined ...
  switch(clk)
  {
    case busClockAHB:
      bitval = (RCC->CFGR & (0x0f << 4)) >> 4;   // HPRE [7:4] to lower 4 bits
      if (bitval & 0x8)           // 1xxx
        divider = 1 << ((bitval & 0x07) + 1);   // 0 = /2, 1 = /4
      else
        divider = 1;              // 0xxx = not divided
      break;
    case busClockAPB1:
    case timersClockAPB1:         // x2
      bitval = (RCC->CFGR & (0x07 << 10)) >> 10; // PPRE1 [12:10] to lower 3 bits
      if (bitval & 0x4)           // 1xx
        divider = 1 << ((bitval & 0x03) + 1);   // 0 = /2, 1 = /4
      else
        divider = 1;              // 0xx = not divided

      break;
    case busClockAPB2:
    case timersClockAPB2:         // the same
      bitval = (RCC->CFGR >> 13) & 0x07; // PPRE2 [15:13] to lower 3 bits
      if (bitval & 0x4)           // 1xx
        divider = 1 << ((bitval & 0x03) + 1);   // 0 = /2, 1 = /4
      else
        divider = 1;              // 0xx = not divided
      break;
    default:
      return 0;
  }

  SystemCoreClockUpdate();      // for sure recalculate SystemCoreClock

  if (((clk == timersClockAPB1) || (clk == timersClockAPB1)) && (divider > 1))
    return SystemCoreClock / divider * 2;
  else
    return SystemCoreClock / divider;
#else
#error Valid controller not set - GetBusClock
#endif
}

bool SystemClock_100MHz(eClockSources clkSrc)
{
  uint32_t t;

#if defined(STM32F411xE)  // || other models with the same CR/CFGR layout
#else
#error Unsupported processor
#endif

#if HSE_VALUE != 8000000
#error HSE_VALUE must be set to 8M = ext. clock from ST/Link on Nucleo
#endif

  if (clkSrc == clockSourceHSE)
  {
    if (!(RCC->CR & RCC_CR_HSEON))      // HSE not running ?
    {
      RCC->CR |= RCC_CR_HSEON;          // enable

      t = 200;
      while(!(RCC->CR & RCC_CR_HSEON) && t)   // wait to ON
        t--;
      if (!t)
        return false;
    }
  }

  if (!(RCC->CR & RCC_CR_HSION))    // HSI not running ?
  {
    RCC->CR |= RCC_CR_HSION;        // enable

    t = 100;
    while(!(RCC->CR & RCC_CR_HSION) && t)   // wait to ON
      t--;
    if (!t)
      return false;
  }

  if (RCC->CR & RCC_CR_PLLON)     // running ?
  {
    RCC->CR &= ~RCC_CR_PLLON;     // stop it
  }

  RCC->CFGR &= ~(RCC_CFGR_SW);    // SW = 00 - HSI as source

  RCC->CFGR = 0;                  // RESET state, all off

  RCC->CFGR |= 0
      | 0 << 13                   // PPRE2 [15:13] = 0xx = not divided
// will be set later ...      | 4 << 10        // PPRE1 [12:10] = 100 = /2 (max. 50MHz)
      | 0 << 4                    // HPRE  [7:4] = 0xxx = not divided
      ;

  //TODO for univerzal using check AHB clock >= 42M (I2C limit]
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;  // 100 = /4 - APB1 can be 50MHz (eg. /2), but I2C cannot

  if (clkSrc == clockSourceHSI)
  {
    RCC->PLLCFGR &= ~(1 << 22);   // PLLSRC [22] = 0 = HSI as source

    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLM;
    RCC->PLLCFGR |= 8 << 0;       // PLLM [5:0] = divider value
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLN;
    RCC->PLLCFGR |= 100 << 6;     // PLLN [14:6] = divider value
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLP;
    RCC->PLLCFGR |= 0 << 16;      // PLLP [17:16] = 00 = /2
  }

  if (clkSrc == clockSourceHSE)
  {
    RCC->PLLCFGR &= ~(1 << 22);   // PLLSRC [22] = 0 = HSI as source
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;

    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLM;
    RCC->PLLCFGR |= 4 << 0;       // PLLM [5:0] = divider value
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLN;
    RCC->PLLCFGR |= 100 << 6;     // PLLN [14:6] = divider value
    RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLP;
    RCC->PLLCFGR |= 0 << 16;      // PLLP [17:16] = 00 = /2
  }

  RCC->CR |= RCC_CR_PLLON;          // enable

  t = 100;
  while(!(RCC->CR & RCC_CR_PLLON) && t)   // wait to ON
    t--;
  if (!t)
    return false;

  // RM - 3.4 Read interface
  FLASH->ACR &= ~(0x0f << 0);     // LATENCY [3:0] = 0000
  FLASH->ACR |= (3 << 0);         // 3 WS

  PWR->CR |= PWR_CR_VOS_0 | PWR_CR_VOS_1;   // scale mode 1 - req. for 100MHz

  RCC->CFGR |= RCC_CFGR_SW_PLL;
  t = 1000;                       // longer waiting ...
  while(!((RCC->CFGR & 0x0c) == RCC_CFGR_SWS_PLL) && t)   // wait to verify SWS
    t--;
  if (!t)
    return false;

  return true;
}

