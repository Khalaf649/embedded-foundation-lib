//
// Created by Khalaf on 29/04/2026.
//

#ifndef USART_H
#define USART_H
#include "../include/STD_TYPES.h"


void Usart1_Init(void);

uint8 Usart1_TransmitByte(uint8 Byte);

uint8 Usart1_RecieveByte(void);

void Usart1_TransmitString(const char* Str);
#endif //USART_H
