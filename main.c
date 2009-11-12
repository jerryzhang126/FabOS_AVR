#include "FabOS.h"


void Task1(void);
void Task2(void);
void Task3(void);
void CPU_init(void);

#ifdef TESTSUITE
uint8_t Task1Stack[200] ;
uint8_t Task2Stack[200] ;
uint8_t Task3Stack[200] ;

uint16_t r,s,t;
#endif

int main(void)
{
#ifdef TESTSUITE
    OS_TaskCreate(0, Task1, Task1Stack);
    OS_TaskCreate(1, Task2, Task2Stack);
    OS_TaskCreate(2, Task3, Task3Stack);
#endif
	CPU_init();

	OS_StartExecution() ;
	while(1)
	{
		// THIS IS the idle task which will be preemted by all other tasks.
		// NO OS-wait-API allowed here!!!
#ifdef TESTSUITE 
#if USECHECKS ==1
		r = OS_get_unused_Stack (Task1Stack);
		s = OS_get_unused_Stack (Task2Stack);
		t = OS_get_unused_Stack (Task3Stack);
#endif
#endif
	}

}

void OS_CustomISRCode(void)
{
	// TODO add your Timer ISR here
	TCNT1 =0;  // reset the timer on ISR
}

void CPU_init(void)
{
	
	
	// init timer
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000011; //250kHZ timer ck
	OCR1A  = 2500; //interrupt every 10ms
	TIMSK |= 1<<OCIE1A; // Output Compare Interrupt ON

	
	//Timer0 Initializations for ATMEGA16
	//TCCR0 |= 5;  // Enable TMR0, set clock source to CLKIO/1024. Interrupts @ 32.768ms intervals @ 8 MHz. This means tasks can execute at least 130,000 instructions before being preempted.
	//TIMSK |= 1 ; // Interrupt on TMR0 Overflow.

}


