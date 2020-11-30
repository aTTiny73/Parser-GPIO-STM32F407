#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include <string.h>

// commandParameters structure used to store all the command parameters
struct commandParameters
{
	char param1[5]; //pbtn,led,help
	char param2[5]; // s(set), b(blink), p(pwm)
	char param3[5]; // N(led num)
	char param4[128]; // S(0,1), O(ms), D(0-100)
	char param5[128]; // P(ms)
};

// ledBlinkParameters strucure used to store led blink task params
struct ledBlinkParameters
{
	int ledNum,onTime,period;
	volatile uint32_t onTimeTriggered;
};


void parseCommand(char*,struct commandParameters*);
void ledOnOff(int,int,struct ledBlinkParameters*,char ,int );
void ledBlink(struct ledBlinkParameters*,uint32_t*,uint32_t*,uint32_t*,uint32_t*);
void ledPWM(int, int,struct ledBlinkParameters*);
void pbtn (char port,int pinNum);

int main(void)
{

	initUSART2(USART2_BAUDRATE_115200);
	enIrqUSART2();
	initSYSTIM();

	struct commandParameters commandParams={0};
	struct ledBlinkParameters ledBlinkParams={0};


	
	//------------------------------------------------------------------
	/// initialize 4 LED's on the board 
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;  
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;								// enable the clock for GPIOD
                							// set PORTD pin 12,13,14,15 as output
    GPIOD->OTYPER = 0x00000000;											// Output push-pull 
    GPIOD->OSPEEDR |= 0xFF000000; 
	GPIOD->ODR &= ~(0x000F << 12);

	GPIOC->OTYPER = 0x00000000;											// Output push-pull 
    GPIOC->OSPEEDR |= 0xFF000000;

	//------------------------------------------------------------------
	//PWM initilization
		RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; 							// enable TIM3 on APB1 
		TIM3->PSC = 0x0054 - 0x0001;									// set TIM3 counting prescaler 
																		// 84MHz/84 = 1MHz -> count each 1us
		TIM3->ARR = 0x03E8;												// period of the PWM 1ms
		
		TIM3->CCR1 = 0x0000;											// duty cycle for the PWM set to 0%
		TIM3->CCR2 = 0x0000;
		TIM3->CCR3 = 0x0000;
		TIM3->CCR4 = 0x0000;
		
		TIM3->CCMR1 |= (TIM_CCMR1_OC1PE)|(TIM_CCMR1_OC1M_2)|(TIM_CCMR1_OC1M_1);
		TIM3->CCMR1 |= (TIM_CCMR1_OC2PE)|(TIM_CCMR1_OC2M_2)|(TIM_CCMR1_OC2M_1);	
		TIM3->CCMR2 |= (TIM_CCMR2_OC3PE)|(TIM_CCMR2_OC3M_2)|(TIM_CCMR2_OC3M_1);	
		TIM3->CCMR2 |= (TIM_CCMR2_OC4PE)|(TIM_CCMR2_OC4M_2)|(TIM_CCMR2_OC4M_1);					
																			// set PWM 1 mod, enable OC1PE preload mode 
																			
		// set active mode high for pulse polarity
		TIM3->CCER &= ~((TIM_CCER_CC1P)|(TIM_CCER_CC2P)|(TIM_CCER_CC3P)|(TIM_CCER_CC4P));
		TIM3->CR1 |= (TIM_CR1_ARPE)|(TIM_CR1_URS);
		
		// update event, reload all config 
		TIM3->EGR |= TIM_EGR_UG;											
		// activate capture compare mode
		TIM3->CCER |= (TIM_CCER_CC1E)|(TIM_CCER_CC2E)|(TIM_CCER_CC3E)|(TIM_CCER_CC4E);
		// start counter										
		TIM3->CR1 |= TIM_CR1_CEN;
	//------------------------------------------------------------------

	uint32_t led_timer1 = getSYSTIM();
	uint32_t led_timer2 = getSYSTIM();
	uint32_t led_timer3 = getSYSTIM();
	uint32_t led_timer4 = getSYSTIM();
	struct ledBlinkParameters ledBlinkTask[5]={0,0,0,0};
	char port = 'C';
	int pinNum = 6;
	char PBTNport = 'A';
	int PBTNpinNum = 0;
	while(1)
	{
	 	if(dataReady == '1'){
			parseCommand(g_usart2_buffer,&commandParams);
			printUSART2("\n");
	  		dataReady = '0';
	 	}else {
		 	if(strcmp(commandParams.param1,"pbtn")==0){
				 pbtn(PBTNport,PBTNpinNum);
			 	commandParams = (struct commandParameters){0};
		 	}else if(strcmp(commandParams.param1,"led")==0){ 
			 	if(strcmp(commandParams.param2,"s")==0){
					int ledNum = atoi(commandParams.param3);
					int ledState = atoi(commandParams.param4);
					if(ledState != 0 && ledState != 1){
						printUSART2("Invalid led state\n");
						commandParams = (struct commandParameters){0};
					}
					ledOnOff(ledNum,ledState,ledBlinkTask,port,pinNum);
					commandParams = (struct commandParameters){0};
				}
				if(strcmp(commandParams.param2,"b")==0){
					ledBlinkParams.ledNum = atoi(commandParams.param3);
					ledBlinkParams.onTime = atoi(commandParams.param4);
					ledBlinkParams.period = atoi(commandParams.param5);
					ledBlinkTask[ledBlinkParams.ledNum]= ledBlinkParams;
					commandParams = (struct commandParameters){0};
				}
				if(strcmp(commandParams.param2,"p")==0){
					int ledNum = atoi(commandParams.param3);
					int ledDutyCycle = atoi(commandParams.param4);
					ledPWM(ledNum,ledDutyCycle,ledBlinkTask);
					commandParams = (struct commandParameters){0};
				}
		 	}else if(strcmp(commandParams.param1,"help")==0){
			 	printUSART2("\e[34m----------------------------------|Help|------------------------------------|\e[39m\n");
				printUSART2("\e[34mCommand: pbtn\e[39m \n Returns current state of user push button. \n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: led s N S , Example: led s 0 1 , led s 1 0\e[39m \n Sets the N(0,1,2,3) led into static state (0,1).\n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: led b N O P , Example: led b 0 300 1000, led b 1 500 1000\e[39m \n Sets the N(0,1,2,3) led into blink mode, O(Ontime ms), P(Period ms). \n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: led p N D , Example: led p 0 50\e[39m \n Sets the N(0,1,2,3) led into PWM mode, D(Duty cycle 0-100).\n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: mapb P , Example: mapb PA3\e[39m \n Sets user defined pin into a input mode.\n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: map1 P , Example: map1 PA3\e[39m \n Changes the output pin of led 0 when calling the (led s 0) command .\n");
				printUSART2("----------------------------------------------------------------------------|\n");
				printUSART2("\e[34mCommand: help\e[39m \n Shows all commands with examples.\n");
				printUSART2("\e[34m----------------------------------------------------------------------------|\e[39m \n");
				commandParams = (struct commandParameters){0};
		 	}else if (strcmp(commandParams.param1,"map1")==0){
				 port = commandParams.param2[1];
				 char tmp[3] = {commandParams.param2[2],commandParams.param2[3]};
				 pinNum = atoi(tmp);
				 commandParams = (struct commandParameters){0};
			 }
			else if (strcmp(commandParams.param1,"mapb")==0){
				 PBTNport = commandParams.param2[1];
				 char tmp[3] = {commandParams.param2[2],commandParams.param2[3]};
				 PBTNpinNum = atoi(tmp);
				 commandParams = (struct commandParameters){0};
			 }
			
			ledBlink(ledBlinkTask,&led_timer1,&led_timer2,&led_timer3,&led_timer4);
	 	}
		 
	}
}

void parseCommand(char* buffer,struct commandParameters* commandParams){
	char *slice = strtok(buffer, " ");
	int i = 0;
	while(slice != NULL){

        ++i;
		if(i==6){
			break;
		}
		
		if (i==1){
		  	strcpy(commandParams->param1,slice);
			commandParams->param1[4]='\0';
			if(strcmp(commandParams->param1,"pbtn")!=0 && strcmp(commandParams->param1,"led")!=0 && strcmp(commandParams->param1,"help")!=0 && strcmp(commandParams->param1,"map1")!=0 && strcmp(commandParams->param1,"mapb")!=0){
				printUSART2("Invalid command\n");
				break;
			}
		}else if(i==2){
			if(strcmp(slice,"s")!=0 && strcmp(slice,"b")!=0 && strcmp(slice,"p")!=0){
				if(strcmp(commandParams->param1,"led")==0){
					printUSART2("Invalid command\n");
					break;
				}
			}
		  	strcpy(commandParams->param2,slice);
			commandParams->param2[4]='\0';
		}else if(i==3){
			if(strcmp(slice,"0")!=0 && strcmp(slice,"1")!=0 && strcmp(slice,"2")!=0 && strcmp(slice,"3")!=0){
				printUSART2("Invalid led number\n");
				break;
			}
		  	strcpy(commandParams->param3,slice);
			commandParams->param3[4]='\0';
		}else if(i==4){
		  	strcpy(commandParams->param4,slice);
			commandParams->param4[127]='\0';
		}else if(i==5){
		  	strcpy(commandParams->param5,slice);
			commandParams->param5[127]='\0';
		} 
		slice = strtok(NULL, " ");
	}
  //buffer not valid after strtok needs to be cleared
  strcpy(buffer, "");
}

void ledOnOff(int ledNum,int ledState,struct ledBlinkParameters* ledBlinkTask,char port,int pinNum){

	ledBlinkTask[ledNum]=(struct ledBlinkParameters){0};

	if(ledNum == 0){

		switch (port)
		{
		case 'A' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
			GPIOA->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOA->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOA->ODR|=(0x1<<pinNum);
			}else{
				GPIOA->ODR&= ~(0x1<<pinNum);
			}
			break;
		case 'B' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
			GPIOB->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOB->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOB->ODR|=(0x1<<pinNum);
			}else{
				GPIOB->ODR&= ~(0x1<<pinNum);
			}
			break;
		case 'C' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
			GPIOC->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOC->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOC->ODR|=(0x1<<pinNum);
			}else{
				GPIOC->ODR&= ~(0x1<<pinNum);
			}
			break;

		case 'D' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
			GPIOD->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOD->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOD->ODR|=(0x1<<pinNum);
			}else{
				GPIOD->ODR&= ~(0x1<<pinNum);
			}
			break;

		case 'E' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
			GPIOE->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOE->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOE->ODR|=(0x1<<pinNum);
			}else{
				GPIOE->ODR&= ~(0x1<<pinNum);
			}
			break;

		case 'F' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
			GPIOF->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			GPIOF->MODER |= (0x1<<(2*pinNum));
			if(ledState==1){
				GPIOF->ODR|=(0x1<<pinNum);
			}else{
				GPIOF->ODR&= ~(0x1<<pinNum);
			}
			break;
		
		default:
			printUSART2("Invalid port\n");
			break;
		}
	}
	else{

		GPIOC->MODER &= ~((0x80000000>>(18-(2*ledNum))) | (0x80000000>>(19-(2*ledNum)))); 
		GPIOC->MODER |= (0x80000000>>(19-(2*ledNum)));
	
		if(ledState==1){
			GPIOC->ODR|=(0x1<<(ledNum+6));
		}else{
			GPIOC->ODR&= ~(0x1<<(ledNum+6));
		}
	}
}

void ledPWM(int ledNum, int dutyCycle,struct ledBlinkParameters* ledBlinkTask){

	ledBlinkTask[ledNum]=(struct ledBlinkParameters){0};

	GPIOC->MODER &= ~((0x80000000>>(18-(2*ledNum))) | (0x80000000>>(19-(2*ledNum)))); 
	GPIOC->MODER |= (0x80000000>>(18-(2*ledNum)));

	if (dutyCycle > 100){
		dutyCycle = 100;
	}else if(dutyCycle < 0){
		dutyCycle = 0;
	}

	if(ledNum == 0){
		GPIOC->AFR[0] &= ~(0x02000000);
		GPIOC->AFR[0] |= 0x02000000;
		TIM3->CCR1 = dutyCycle * 10;
	}else if(ledNum == 1){
		GPIOC->AFR[0] &= ~(0x20000000);
		GPIOC->AFR[0] |= 0x20000000;
		TIM3->CCR2 = dutyCycle * 10;
	}else if(ledNum == 2){
		GPIOC->AFR[1] &= ~(0x00000002);
		GPIOC->AFR[1] |= 0x00000002;
		TIM3->CCR3 = dutyCycle * 10;
	}else if(ledNum == 3){
		GPIOC->AFR[1] &= ~(0x00000020);
		GPIOC->AFR[1] |= 0x00000020;
		TIM3->CCR4 = dutyCycle * 10;
	}
	
}

void ledBlink(struct ledBlinkParameters* ledBlinkTask,uint32_t* led_timer1,uint32_t* led_timer2,uint32_t* led_timer3,uint32_t* led_timer4){

	if((ledBlinkTask[0]).ledNum == 0 && (ledBlinkTask[0]).period!=0){

		GPIOC->MODER &= ~((0x80000000>>(18-(2*(ledBlinkTask[0].ledNum)))) | (0x80000000>>(19-(2*(ledBlinkTask[0].ledNum))))); 
		GPIOC->MODER |= (0x80000000>>(19-(2*(ledBlinkTask[0].ledNum))));

		if(chk4TimeoutSYSTIM(*led_timer1, ledBlinkTask[0].onTime, ledBlinkTask[0].period,&(ledBlinkTask[0].onTimeTriggered)) == (SYSTIM_TIMEOUT)){

			GPIOC->ODR ^= (0x1<<((ledBlinkTask[0].ledNum)+6));
			*led_timer1 = getSYSTIM();
		}
	}
	if((ledBlinkTask[1]).ledNum == 1) {

		GPIOC->MODER &= ~((0x80000000>>(18-(2*(ledBlinkTask[1].ledNum)))) | (0x80000000>>(19-(2*(ledBlinkTask[1].ledNum))))); 
		GPIOC->MODER |= (0x80000000>>(19-(2*(ledBlinkTask[1].ledNum))));

		if(chk4TimeoutSYSTIM(*led_timer2, ledBlinkTask[1].onTime, ledBlinkTask[1].period,&ledBlinkTask[1].onTimeTriggered) == (SYSTIM_TIMEOUT)){

			GPIOC->ODR ^= (0x1<<((ledBlinkTask[1].ledNum)+6));
			*led_timer2 = getSYSTIM();
		}
	}
	if((ledBlinkTask[2]).ledNum == 2 ){

		GPIOC->MODER &= ~((0x80000000>>(18-(2*(ledBlinkTask[2].ledNum)))) | (0x80000000>>(19-(2*(ledBlinkTask[2].ledNum))))); 
		GPIOC->MODER |= (0x80000000>>(19-(2*(ledBlinkTask[2].ledNum))));

		if(chk4TimeoutSYSTIM(*led_timer3, ledBlinkTask[2].onTime, ledBlinkTask[2].period,&ledBlinkTask[2].onTimeTriggered) == (SYSTIM_TIMEOUT)){

			GPIOC->ODR ^= (0x1<<((ledBlinkTask[2].ledNum)+6));
			*led_timer3 = getSYSTIM();
		}
	}
	if((ledBlinkTask[3]).ledNum == 3 ){

		GPIOC->MODER &= ~((0x80000000>>(18-(2*(ledBlinkTask[3].ledNum)))) | (0x80000000>>(19-(2*(ledBlinkTask[3].ledNum))))); 
		GPIOC->MODER |= (0x80000000>>(19-(2*(ledBlinkTask[3].ledNum))));

		if(chk4TimeoutSYSTIM(*led_timer4, ledBlinkTask[3].onTime, ledBlinkTask[3].period,&ledBlinkTask[3].onTimeTriggered) == (SYSTIM_TIMEOUT)){

			GPIOC->ODR ^= (0x1<<((ledBlinkTask[3].ledNum)+6));
			*led_timer4 = getSYSTIM();
		}
	}	
	
}

void pbtn (char port,int pinNum){
	switch (port)
		{
		case 'A' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
			GPIOA->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum)));
			printUSART2("Push button state: %d\n",GPIOA->IDR & 0x1<<pinNum);
			break;

		case 'B' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
			GPIOB->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			printUSART2("Push button state: %d\n",GPIOB->IDR & 0x1<<pinNum);
			break;

		case 'C' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
			GPIOC->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			printUSART2("Push button state: %d\n",GPIOC->IDR & 0x1<<pinNum);
			break;

		case 'D' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
			GPIOD->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			printUSART2("Push button state: %d\n",GPIOD->IDR & 0x1<<pinNum);
			break;

		case 'E' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
			GPIOE->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			printUSART2("Push button state: %d\n",GPIOE->IDR & 0x1<<pinNum);
			break;

		case 'F' :
			RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
			GPIOF->MODER &= ~((0x1<<(2*pinNum)) | (0x2<<(2*pinNum))); 
			printUSART2("Push button state: %d\n",GPIOF->IDR & 0x1<<pinNum);
			break;
		
		default:
			printUSART2("Invalid port\n");
			break;
		}
}