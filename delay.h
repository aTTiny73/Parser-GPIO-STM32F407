#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

#define SYSTIMER_TIMEOUT						0
#define SYSTIMER_KEEP_ALIVE						1
#define SYSTIM_TIMEOUT							0
#define SYSTIM_KEEP_ALIVE						1

void 		delay_ms(uint32_t ms);				// 													
void 		delay_us(uint32_t ms);				// 


void 		initSTOPWATCH(void);
void 		startSTOPWATCH(void);
uint32_t 	stopSTOPWATCH(void);

void 		initSYSTIMER(void);
uint32_t 	getSYSTIMER(void);
uint8_t 	chk4TimeoutSYSTIMER(uint32_t btime, uint32_t period);

void 		initSYSTIM(void);
uint32_t 	getSYSTIM(void);
uint8_t 	chk4TimeoutSYSTIM(uint32_t btime,uint32_t onTime, uint32_t period,volatile uint32_t* onTimeTriggered);

extern volatile uint32_t g_tim7_val;
#endif 
