/*
 * nucleo_usart.h
 *
 *  Created on: 17. 3. 2017
 *      Author: Petr
 */

#ifndef NUCLEO_USART_H_
#define NUCLEO_USART_H_

#include "stm_core.h"
#include <stdio.h>

int Usart2Send(char c);
void Usart2String(char *txt);
int Usart2Recv(void);
bool IsUsart2Recv(void);   // flag if something in buffer

void Usart2Init(int baudSpeed);

#endif /* NUCLEO_USART_H_ */
