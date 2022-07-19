/*
 * nucleo_usart.c
 *
 *  Created on: 17. 3. 2017
 *      Author: Petr
 */

#include <stm_usart.h>

// CubeIDE new functionality begin
int __io_putchar(int ch) // in syscalls declared as __attribute__((weak));
{
  return Usart2Send(ch);
}
int __io_getchar(void)  // weak declaration too
{
  return Usart2Recv();
}
// CubeIDE new functionality end

static bool _autoCR = true;             // internal flag

int Usart2Send(char c)
{
  if ((c == '\n') && _autoCR)
  {
    while(!(USART2->SR & USART_SR_TXE))
      ;
    USART2->DR = '\r';                 // write CR before LF
  }

  while(!(USART2->SR & USART_SR_TXE)) // wait for TDR empty
    ;
  USART2->DR = c;                     // write to TDR to send (TXE clears automatically)
  return c;
}

void Usart2String(char *txt)
{
  while(*txt)
  {
    Usart2Send(*txt);
    txt++;
  }
}

int Usart2Recv(void)
{
  while(!(USART2->SR & USART_SR_RXNE))  // wait while something received in RDR
    ;

  return USART2->DR;      // read and return value (RXNE clears automatically)
}

bool IsUsart2Recv(void)   // flag if something in recv. buffer
{
  return (USART2->SR & USART_SR_RXNE) != 0; // enforce true/false
}

void Usart2Init(int baudSpeed)
{
  STM_SetPinGPIO(GPIOA, 2, ioPortAlternatePP);
  STM_SetAFGPIO(GPIOA, 2, 7);    // AF7 is USART2
  STM_SetPinGPIO(GPIOA, 3, ioPortAlternatePP);
  STM_SetAFGPIO(GPIOA, 3, 7);    // AF7 is USART2

  if (!(RCC->APB1ENR & RCC_APB1ENR_USART2EN))       // not working USART2 ?
  {
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;
  }

  USART2->CR1 = USART_CR1_RE | USART_CR1_TE;      // enable only receive and send/transmit
  USART2->CR2 = 0;      // nothing special
  USART2->CR3 = 0;

  // USART2->BRR = 0x1A1; // speed 38400 by 16MHz - pre-calculated
  {
    uint sampling = (USART2->CR1 & USART_CR1_OVER8) ? 8 : 16;
    uint32_t apb1, mant, tmp, frac;
    apb1 = STM_GetBusClock(busClockAPB1);

    mant = apb1 * 16 / (sampling * baudSpeed);    // part of 16th
    tmp = mant / 16;

    frac = mant - (tmp * 16);                     // remain after 16 division
    USART2->BRR = (tmp << 4) | (frac & 0x0f);
  }

  USART2->CR1 |= USART_CR1_UE;    // enable USART block after all settings

  // stop default (line) buffering of output and input
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
}

