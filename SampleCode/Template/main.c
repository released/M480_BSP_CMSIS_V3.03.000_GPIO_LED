/*************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    A project template for M480 MCU.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "NuMicro.h"

#define PLL_CLOCK           		192000000

#define BUF_LEN					(1024)

//#define LED_TIMER_200K
#define LED_TIMER_10K

uint32_t LED1_R, LED1_G, LED1_B, Blink ,brea=0,LED_cnt=0,brea_cnt=0;

typedef struct {
	uint8_t buf[BUF_LEN];
	uint16_t len;
	uint8_t end;
}UART_BUF_t;
UART_BUF_t uartDev;

extern void SYS_Init(void); 

void RYG_BreathDimming_200K(void)
{

	#define INC_VALUE   		5
	#define PWM_WIDTH    	1500

    static uint8_t state=0;   
	static uint16_t led_i = 0 ;
	static uint16_t led_H = 0 ;
	static uint16_t led_W = 0 ;

    ++led_i;
     if(led_i < led_H)
     {
		PH0=1;
		PH1=1;
		PH2=1;
     }
     else if((led_i >= led_H) && (led_i < led_W))
     {
		PH0=0; 
		PH1=0;
		PH2=0;
     }
     else
     {
         led_i = 0;
         led_W = PWM_WIDTH;
         if(state==0)
         {
             led_H += INC_VALUE;
             if(led_H >= PWM_WIDTH)
             {
                 state = 1;
             }
         }
         else
         {            
             led_H -= INC_VALUE;
             if(led_H <= 100)
             {
                 state = 0;
             }
         }
     }
 
}

void RYG_BreathDimming_10K(void)
{
	/*
		EC_M451_RGBLED_Dimming_V1.0
		https://www.nuvoton.com/hq/resource-download.jsp?tp_GUID=EC0120160725162719
	*/
   	uint32_t LED_duty,RLED,BLED,GLED,LED_brea;

	LED_cnt++;
	LED_duty=LED_cnt%100;
	if ((brea==1)&&((brea_cnt%2)==0))
			LED_brea=100-(LED_cnt/100);
	else if (brea==1)
			LED_brea=LED_cnt/100;
	else
			LED_brea=0;
	
	RLED=((int32_t)(LED1_R-LED_duty-LED_brea)>0)?1:0;
	BLED=((int32_t)(LED1_B-LED_duty-LED_brea)>0)?1:0;
	GLED=((int32_t)(LED1_G-LED_duty-LED_brea)>0)?1:0;	

	if (LED_cnt>=(Blink*1000))
	{
		RLED=0;
		BLED=0;
		GLED=0;
	}
		
	PH->DOUT = (PH->DOUT|BIT0|BIT1|BIT2)&(~((RLED<<0)|(GLED<<1)|(BLED<<2)));
	if (LED_cnt==10000)
	{
		LED_cnt=0;
		brea_cnt++;
	}
}

void RYG_Init_10K(void)
{
	LED1_R=100;
	LED1_G=100;
	LED1_B=100;
	Blink=10;
	brea=1;
}

void TMR0_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
        TIMER_ClearIntFlag(TIMER0);

		#if defined (LED_TIMER_200K)
		{
			RYG_BreathDimming_200K();
		}
		#elif defined (LED_TIMER_10K)
		{
			RYG_BreathDimming_10K();
		}
		#endif
    }
}

void LEDTimer_TIMER0_Init(void)
{
	#if defined (LED_TIMER_200K)
	{
		TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 200000);
	}
	#elif defined (LED_TIMER_10K)
	{
    	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 10000);
		RYG_Init_10K();
	}
	#endif

    TIMER_EnableInt(TIMER0);
    NVIC_EnableIRQ(TMR0_IRQn);	
    TIMER_Start(TIMER0);
}


void TMR3_IRQHandler(void)
{

//	static uint16_t cnt_gpio = 0;

	static uint32_t LOG = 0;
	static uint16_t CNT = 0;

    if(TIMER_GetIntFlag(TIMER3) == 1)
    {
        TIMER_ClearIntFlag(TIMER3);

		if (CNT++ >= 1000)
		{
			CNT = 0;
        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
		}

//		if (cnt_gpio++ >= 1000)
//		{
//			cnt_gpio = 0;
//			GPIO_TOGGLE(PH1);
//			PH1 = ~PH1;
//		}
		

    }
}

void BasicTimer_TIMER3_Init(void)
{
    TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER3);
    NVIC_EnableIRQ(TMR3_IRQn);	
    TIMER_Start(TIMER3);
}

void GPIO_Init(void)
{
    GPIO_SetMode(PH, BIT0, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PH, BIT1, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PH, BIT2, GPIO_MODE_OUTPUT);	
}

void UART0_IRQHandler(void)
{
	if (UART_GET_INT_FLAG(UART0,UART_INTSTS_RDAINT_Msk))
	{
		while(!UART_GET_RX_EMPTY(UART0))
		{
			uartDev.buf[uartDev.len++] = UART_READ(UART0);
		}
	}
	
	if (UART_GET_INT_FLAG(UART0,UART_INTSTS_RXTOIF_Msk))
	{
		while(!UART_GET_RX_EMPTY(UART0))
		{
			uartDev.buf[uartDev.len++] = UART_READ(UART0);
		}
		
		uartDev.end = 1;
	}
}

void UART0_Process(void)
{

	/*
		EC_M451_UART_Timerout_V1.00.zip
		https://www.nuvoton.com/hq/resource-download.jsp?tp_GUID=EC0120160728090754
	*/
	
	if (uartDev.end)
	{
		while(!UART_GET_RX_EMPTY(UART0))
		{
			uartDev.buf[uartDev.len++] = UART_READ(UART0);
		}

		#if 1
		printf("%s : %d\r\n",__FUNCTION__,uartDev.len);

		#endif

		UART_Write(UART0,uartDev.buf,uartDev.len);
		
		memset(&uartDev, 0x00, sizeof(UART_BUF_t));
	}

}


void UART0_Init(void)
{
    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	
	/* Set UART FIFO RX interrupt trigger level to 4-bytes*/
	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);
	
	memset(&uartDev, 0x00, sizeof(UART_BUF_t));

	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());
//	printf("%2d\r\n",PDMATIMEOUT(1));
	
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{

    SYS_Init();
	
    UART0_Init();

	GPIO_Init();
	
	BasicTimer_TIMER3_Init();

	LEDTimer_TIMER0_Init();
	
    /* Got no where to go, just loop forever */
    while(1)
    {
		UART0_Process();
    }

}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
