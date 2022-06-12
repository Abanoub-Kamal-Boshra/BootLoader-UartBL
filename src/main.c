/*
 * main.c
 *
 *  Created on: Oct 13, 2020
 *      Author: Ahmed
 */


#include "STD_TYPES.h"
#include "BIT_MATH.h"

#include "RCC_interface.h"
#include "DIO_interface.h"
#include "STK_interface.h"
#include "USART_interface.h"
#include "FPEC_interface.h"

void Parser_voidParseRecord(u8* Copy_u8BufData);

volatile u8  u8RecBuffer[100]   ;
volatile u8  u8RecCounter    = 0;
volatile u8  u8TimeOutFlag   = 0;
volatile u16 u16TimerCounter = 0;
volatile u8  u8BLWriteReq    = 1;

/* typedef --> pointer to function */
typedef void (*Function_t)(void);
/* create object from this type */
Function_t addr_to_call = 0;



void func(void)
{
#define SCB_VTOR   *((volatile u32*)0xE000ED08)

	SCB_VTOR = 0x08001000;
	/* the first address in the flash (page_4 --> app area) 0x08001000
	 * but we use 0x08001004 as the first address contain the reset address (that initialize the stack pointer) */
	/* cast the address as pointer to function and put the value of pointee to a variable
	 * for ex: var = *(int*)y      */
	addr_to_call = *(Function_t*)(0x08001004);
	/* jump to application */
	addr_to_call();
}
/* F:\Embded system\ARM\workspace\LED\Debug\LED.txt */

void main(void)
{
	u8 Local_u8RecStatus;

	RCC_voidInitSysClock();  /* enable HSI */
	RCC_voidEnableClock(RCC_APB2,14); /* USART1 */
	RCC_voidEnableClock(RCC_APB2,2);  /* PortA  */
	RCC_voidEnableClock(RCC_AHB,4);   /* FPEC   */


	MGPIO_VidSetPinDirection(GPIOA,9,0b1010);   /* TX AFPP */
	MGPIO_VidSetPinDirection(GPIOA,10,0b0100);  /* RC Input Floating */

	MUSART1_voidInit(); /* Baud rate 9600 - enable TX and RX */

	MSTK_voidInit();

	MSTK_voidSetIntervalSingle(5000000,func);  // ASYNC

	/* we can't put the flash erase code here as
	 * we can reset the micro without we need to flash a new application
	 * so if we erase here ---> we will lose the current application */
	while(u8TimeOutFlag == 0)
	{

		Local_u8RecStatus = MUSART1_u8Receive( &(u8RecBuffer[u8RecCounter]) );
		if (Local_u8RecStatus == 1)
		{
			/* stop the timer */
			MSTK_voidStopInterval();

			if(u8RecBuffer[u8RecCounter] == '\n')
			{
				/* #erase the flash(page4 : page63) at the first recored received only
				 *  after we receive the all the first recored as in this case
				 *  the tool will be stop till we send to it 'ok'
				 * #but if we erase after we receive only the first char of the recored (that tack long time)
				 *  and in the same time the usart send the the other chars of the recored
				 *  in this case we can't receive these chars
				 *  and i suggest that: may be the usart send all file before erase function ended that will kill the flash operation */
				if (u8BLWriteReq == 1)
				{
					FPEC_voidEraseAppArea();
					u8BLWriteReq = 0;
				}

				/* Parse */
				Parser_voidParseRecord(u8RecBuffer);
				MUSART1_voidTransmit("ok");
				u8RecCounter = 0;
			}

			else
			{
				u8RecCounter ++ ;
			}

			MSTK_voidSetIntervalSingle(5000000,func);
		}

		else
		{
			/* Nothing to receive */
		}
	}
}
