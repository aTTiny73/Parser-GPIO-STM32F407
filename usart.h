#ifndef __USART2_H_
#define __USART2_H_

#include <stdio.h>
#include <stdarg.h>
#include "stm32f4xx.h"
#include "misc.h"



#define USART2_BUFFER_SIZE			512

char g_usart2_buffer[USART2_BUFFER_SIZE];
uint8_t dataReady;


#define USART2_BAUDRATE_921600		0x0000002D
#define USART2_BAUDRATE_460800		0x0000005B
#define USART2_BAUDRATE_115200		0x0000016C
#define USART2_BAUDRATE_9600		0x00001117

void initUSART2(uint32_t baudrate);
void putcharUSART2(uint8_t data);
uint8_t getcharUSART2();
void printUSART2(char * str, ... );
void sprintUSART2(uint8_t * str);
void chkRxBuffUSART2(void);
void enIrqUSART2(void);
#endif 
