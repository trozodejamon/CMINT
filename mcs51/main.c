/********************************************************************************
 * MINT Main Source
 ********************************************************************************/
#include <8052.h>
#include "stc15f2k60s2.h"
#include "mint.h"

__bit busy;
__bit bufferFull;
#define CIRBUFSIZE 16
char serBuf[CIRBUFSIZE];
BYTE bufHead, bufCount;
static int msRunTime;

int available(void)
{
  return bufCount;
}

void txChar(char ch)
{
	while(busy);		// If serial port shift out is active, then wait.
	busy = 1;			// Ready to send, indicate that we're busy.
	SBUF = ch;			// Send the byte.
}

BYTE rxChar(void)		// From BufGet
{
	BYTE retVal;						// Preset return value to true case
	if(bufCount<1)						// Invalid parameters or nothing to fetch
		retVal = 0;						// Return false
	else								// Something in the buffer
	{
		retVal = serBuf[bufHead];		// Send it back
		bufCount--;						// "Remove" it from the FIFO queue
		bufferFull = 0;					// Thus buffer is no longer full
		bufHead = (++bufHead) & 0x0F;	// Advance the head pointer
	}
	return retVal;
}

CELL_T getMillis(void)
{
	return msRunTime;
}

// Initialise the CPU so we use 32KB external RAM
void MemInit(void)
{
	// Reduce the memory access hold time.
	BUS_SPEED = 0;	// 0 - Only 1 clock cycle hold time.
					// 1 - Try 2 clocks hold time.
}

void Timer0Init(void)		//1ms@11.0592MHz
{
	AUXR |= 0x80;			//Timer clock is 1T mode
	TMOD &= 0xF0;			//Set timer work mode
	TL0 = 0xCD;				//Initial timer value
	TH0 = 0xD4;				//Initial timer value
	TF0 = 0;				//Clear TF0 flag
	TR0 = 1;				//Timer0 start run
	ET0 = 1;				//Enable Timer0 interrupt
}

void UartInit(void)		//9600bps@11.0592MHz
{
	SCON = 0x50;		//8bit and variable baudrate
	AUXR |= 0x01;		//Use Timer2 as baudrate generator
	AUXR |= 0x04;		//Timer2's clock is Fosc (1T)
	T2L = 0xE0;			//Initial timer value
	T2H = 0xFE;			//Initial timer value
	AUXR |= 0x10;		//Timer2 running
	ES = 1;
	EA = 1;
}

/******************************************************************************
 * Interrupt Service Routines
 ******************************************************************************/
// Interrupt vector 1 for Timer0, using register bank (1).
void timer0_isr (void) __interrupt (1) __using (1)
{
	msRunTime++;
}

// Interrupt vector 4 for UART1, using register bank (2).
void uart_isr (void) __interrupt (4) __using (2)
{
	if(RI)
	{
		RI = 0;
		if(!bufferFull)
		{
			serBuf[(bufHead+bufCount) & 0x0F] = SBUF;	// Insert into the circular buffer
			bufCount++;									// Now we update the count.
			bufferFull = (bufCount>=CIRBUFSIZE);		// Update the status.
		}
		// Else there's nothing much we can do.
	}
	if(TI)		// Transmission complete
	{
		TI = 0;
		busy = 0;
	}
}

void main(void)
{
	busy = 0;
	MemInit();
	Timer0Init();
	UartInit();
	mintInit();
	while(1)
	{
		mintRun();
	}
}