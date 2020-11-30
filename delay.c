#include "delay.h"


volatile uint32_t g_tim7_val = 0;

void delay_ms(uint32_t ms)
{/// delay in ms 
 
	RCC->APB1ENR |= RCC_APB1ENR_TIM12EN; 								// 
	TIM12->PSC = 0x0054 - 0x0001;										// APB1@42MHz
																		// 
	TIM12->ARR = 0x03E8;												// reload 1000 us
	TIM12->CR1 = 0x0084;												// ARPE On, CMS disable, Up counting

	TIM12->EGR |= TIM_EGR_UG;											// reload all config p363
	TIM12->CR1 |= TIM_CR1_CEN;											// start counter
	while(ms > 0)
	{
																		
		while((TIM12->SR & TIM_SR_UIF) == 0x0000);						// wait for update event
		
		TIM12->SR &= ~TIM_SR_UIF;										// clear the update event interrupt flag
		ms--;
	} 
	TIM12->CR1 &= ~TIM_CR1_CEN;											// stop counter 
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM12EN;								// disable TIM 
}

void delay_us(uint32_t us)
{/// delay in us 
    
	RCC->APB1ENR |= RCC_APB1ENR_TIM12EN; 								//
	TIM12->PSC = 0x0001 - 0x0001;										// 
																		// 
	TIM12->ARR = 0x0054;												// reload value set to 1 us
	TIM12->CR1 = 0x0084;												// ARPE On, CMS disable, Up counting
																		// UEV disable, TIM4 enable(p392)

	TIM12->EGR |= TIM_EGR_UG;											//reload all config p363
	TIM12->CR1 |= TIM_CR1_CEN;											// start counter
	while(us > 0)
	{													
		while((TIM12->SR & TIM_SR_UIF) == 0x0000);						// wait for update event
		
		TIM12->SR &= ~TIM_SR_UIF;										// clear the update event interrupt flag
		us--;
	} 
	TIM12->CR1 &= ~TIM_CR1_CEN;											// stop counter 
	RCC->APB1ENR &= ~RCC_APB1ENR_TIM12EN;								// disable TIM4 
}

void initSTOPWATCH(void)
{/// us resolution	
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN; 								// enable TIM5 
	TIM5->PSC = 0x0054-0x0001;											// set TIM5 counting prescaler to 84 (p406)
																		// 84MHz/84 = 1MHz -> count each us
	TIM5->ARR = 0xFFFFFFFF;												// reload value set to 1.19304647083h!
	TIM5->CR1 = 0x0084;													// ARPE On, CMS disable, Up counting
																		// UEV disable, TIM5 enable(p392)
	TIM5->CR2 = 0x0000;
	TIM5->CNT = 0x00000000;												// clear the start value
	
}

void startSTOPWATCH(void)
{
	TIM5->EGR |= TIM_EGR_UG;											// update event, reload all config p363
	TIM5->CR1 |= TIM_CR1_CEN;											// start counting!
}

uint32_t stopSTOPWATCH(void)
{
	uint32_t time;

	time = TIM5->CNT;
	TIM5->CR1 &= ~TIM_CR1_CEN;											// stop couting
	return time;
}

void initSYSTIMER(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; 								// 
	TIM2->PSC = 0x0001-0x0001;											// 
																		// 
	TIM2->ARR = 0xFFFFFFFF;												// 
	TIM2->CR1 = 0x0084;													// 
																		//
	TIM2->CR2 = 0x0000;
	TIM2->CNT = 0x00000000;												// 
	TIM2->EGR |= TIM_EGR_UG;											//
	TIM2->CR1 |= TIM_CR1_CEN;											// 	
}

uint32_t getSYSTIMER(void)
{
	uint32_t time = TIM2->CNT;
	return time;
}

uint8_t chk4TimeoutSYSTIMER(uint32_t btime, uint32_t period)
{
	uint32_t time = TIM2->CNT;
	if(time >= btime)
	{
		if(time >= (btime + period))
			return (SYSTIMER_TIMEOUT);
		else
			return (SYSTIMER_KEEP_ALIVE);
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - btime;
		if((time + utmp32) >= period)
			return (SYSTIMER_TIMEOUT);
		else
			return (SYSTIMER_KEEP_ALIVE);
	}
}

void initSYSTIM(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN; 								// 
	TIM7->PSC = 0x0054-0x0001;											// 
																		// 
	TIM7->ARR = 0x03E8;													// 
	TIM7->CR1 = 0x0084;													// 
																		//
	TIM7->CR2 = 0x0000;
	TIM7->CNT = 0x0000;													// 
	TIM7->EGR |= TIM_EGR_UG;											//
	TIM7->DIER = 0x0001;												// enable 
	
	NVIC_SetPriority(TIM7_IRQn, 0);
	NVIC_EnableIRQ(TIM7_IRQn);
	TIM7->CR1 |= TIM_CR1_CEN;											// 	
}

void TIM7_IRQHandler(void)
{
	if(TIM7->SR & 0x0001)
	{
		g_tim7_val++;
		TIM7->SR = 0x0000;
	}
}

uint32_t getSYSTIM(void)
{
	return g_tim7_val;
}
//int onTimeTriggered = 0;
uint8_t chk4TimeoutSYSTIM(uint32_t btime,uint32_t onTime, uint32_t period,volatile uint32_t* onTimeTriggered)
{
	//printUSART2("onTimeTrigger %d \n",*onTimeTriggered);
	uint32_t time = g_tim7_val;
	
	if(time >= btime)
	{
		if(*onTimeTriggered == 0){
			if(time >= (btime + period-onTime)){
				(*onTimeTriggered) = 1;
				//printUSART2("onTime, %d\n",*onTimeTriggered);
				return (SYSTIM_TIMEOUT);
			}
			else{
				return (SYSTIM_KEEP_ALIVE);
			}
		}
		else if (*onTimeTriggered == 1){
			if(time >= (btime + onTime)){
				//printUSART2("Period-OnTime\n");
				*onTimeTriggered = 0;
				return (SYSTIM_TIMEOUT);
			}
			else{
				return (SYSTIM_KEEP_ALIVE);
			}
		}
	}
	else
	{
		uint32_t utmp32 = 0xFFFFFFFF - btime;
		if(*onTimeTriggered == 0){
			if((time + utmp32) >= period-onTime){
				(*onTimeTriggered) = 1;
				//printUSART2("onTime\n");
				return (SYSTIM_TIMEOUT);
			}
			else{
				return (SYSTIM_KEEP_ALIVE);
			}
		}
		else if (*onTimeTriggered == 1){
			if((time + utmp32) >= onTime){
				*onTimeTriggered = 0;
				//printUSART2("Period-OnTime\n");
				return (SYSTIM_TIMEOUT);
			}
			else{
				return (SYSTIM_KEEP_ALIVE);
			}
		}
	}
}

