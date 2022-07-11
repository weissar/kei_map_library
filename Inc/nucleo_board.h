/*
 * nucleo_board.h
 *
 *  Created on: Jan 19, 2022
 *      Author: Weissar
 */

#ifndef COMMON_INC_NUCLEO_BOARD_H_
#define COMMON_INC_NUCLEO_BOARD_H_

#include "stm_i2c.h"
#include "stm_usart.h"
#include "stm_core.h"

#define BOARD_BTN_BLUE  GPIOC,13
#define BOARD_LED GPIOA,5

#define NUCLEO_A0   GPIOA,0         // ADC1/0, PWM2/1, UART2CTS
#define NUCLEO_A1   GPIOA,1         // ADC1/1, PWM2/2, UART2RTS, SPI4MOSI
#define NUCLEO_A2   GPIOA,4         // ADC1/4, SPI3SSEL
#define NUCLEO_A3   GPIOB,0         // ADC1/8, PWM1/2N, SPI5CLK
#define NUCLEO_A4   GPIOC,1         // ADC1/11
#define NUCLEO_A5   GPIOC,0         // ADC1/10

#define NUCLEO_D0   GPIOA,3         // UART2RX (JPx !!)
#define NUCLEO_D1   GPIOA,2         // UART2TX (JPx !!)
#define NUCLEO_D2   GPIOA,10        // PWM1/3, SPI5MOSI, UART1RX
#define NUCLEO_D3   GPIOB,3         // PWM2/2, I2C2SDA, SPI3SCLK, UART1RX
#define NUCLEO_D4   GPIOB,5         // PWM3/2, SPI3MOSI
#define NUCLEO_D5   GPIOB,4         // PWM3/1, SPI3MISO, I2C3SDA
#define NUCLEO_D6   GPIOB,10        // PWM2/3, SPI2CLK, I2C2SCL
#define NUCLEO_D7   GPIOA,8         // PWM1/1, I2C3SCL

#define NUCLEO_D8   GPIOA,9         // PWM1/2, UART1TX
#define NUCLEO_D9   GPIOC,7         // PWM3/2, SPI2SCLK, UART3RX
#define NUCLEO_D10  GPIOB,6         // PWM4/1, I2C1SCL, UART1TX
#define NUCLEO_D11  GPIOA,7         // ADC1/7, PWM1/1N, SPI1MOSI
#define NUCLEO_D12  GPIOA,6         // ADC1/6, PWM3/1, SPI1MISO
#define NUCLEO_D13  GPIOA,5         // ADC1/5, PWM2/1, SPI1SCLK, LED

#define NUCLEO_D14  GPIOB,9         // PWM4/4, SPI2SSEL, I2C1SSDA
#define NUCLEO_D15  GPIOB,8         // PWM4/3, SPI5MOSI, I2C1SCL

#endif /* COMMON_INC_NUCLEO_BOARD_H_ */
