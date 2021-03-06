/*
	FabOS for ATMEL AVR

	(c) 2008-2010 Fabian Huslik

	This is free software according GNU GENERAL PUBLIC LICENSE Version 3.

	In this file there should be no need to change anything.
	If you have to change something, please let the author know via FabOS@huslik.net.

*/
#ifndef FABOS_H
#define FABOS_H



#include <avr/io.h>
#include <avr/interrupt.h>
#include "../FabOS_config.h"

// variable types for more tasks
#if OS_NUMTASKS < 8
#define OS_TypeTaskBits_t  uint8_t
#elif OS_NUMTASKS < 16
#define OS_TypeTaskBits_t  uint16_t
#elif OS_NUMTASKS < 32
#define OS_TypeTaskBits_t  uint32_t
#elif OS_NUMTASKS < 64
#define OS_TypeTaskBits_t  uint64_t
#else
	#error reduce OS_NUMTASKS
#endif

typedef struct OS_Alarm_tag {
	uint8_t 			TaskID; // Task ID to wake up
	OS_TypeAlarmTick_t 	AlarmTicks; // ticks to count down before reactivation
} OS_Alarm_t;

// *********  the OS data struct
typedef struct FabOS_tag
{
#if OS_USECLOCK == 1
	volatile uint32_t    	OSTicks;					// the OS time, to prevent cluttered results, always use the Function OS_GetTicks() to read it.
#endif
	volatile uint8_t		EventMask[OS_NUMTASKS] ;	// The event masks for all the tasks; Index = Task ID // no event for idle task.
	volatile uint8_t		EventWaiting[OS_NUMTASKS]; // The mask indicates the events, the tasks are waiting for. Index = Task ID

	volatile uint8_t 	MutexOwnedByTask[OS_NUMMUTEX] ;	 // Mutex-owner (contains task ID of owner); only one task can own a mutex.
	volatile uint8_t 	MutexTaskWaiting[OS_NUMTASKS+1] ;	// Mutex-waiters (contains mutex ID) ; Index = Task ID ; The last one is the idle task.

	volatile uint8_t 	CurrTask; 				// here the NUMBER of the actual active task is set.
	volatile OS_TypeTaskBits_t	TaskReadyBits ; 			// here the task activation BITS are set. Task 0 (LSB) has the highest priority.
	volatile uint16_t 	Stacks[OS_NUMTASKS+1];		// actual SP position addresses for the tasks AND the IDLE-task, which uses the ordinary stack! Index = Task ID
	volatile OS_Alarm_t	Alarms[OS_NUMALARMS];  // Holds the number of system clock ticks to wait before the task becomes ready to run.

#if OS_USEMEMCHECKS == 1
	volatile uint8_t*     StackStart[OS_NUMTASKS+1];		// Stack start pointers for checker function
#endif
} FabOS_t;

typedef struct OS_Queue_tag {
	  uint8_t read; // field with oldest content
	  uint8_t write; // always empty field
	  uint8_t chunk;	// size of element
	  uint8_t size;		// number of elements
	  uint8_t* data;	// pointer to data
	} OS_Queue_t;

extern FabOS_t MyOS;

// *********  Macros to simplify the API

#define OS_DeclareQueue(NAME,COUNT,CHUNK) uint8_t OSQD##NAME[(COUNT+1)*CHUNK]; OS_Queue_t NAME = {0,0,CHUNK,(COUNT+1)*CHUNK,OSQD##NAME}

#define OS_DeclareTask(NAME,STACKSIZE) void NAME(void); uint8_t OSStack##NAME[STACKSIZE]

#define OS_CreateTask(NAME, PRIO)  OS_TaskCreateInt(NAME, PRIO, OSStack##NAME , sizeof(OSStack##NAME))

#define OS_CreateAlarm(ALARMID, TASKID) MyOS.Alarms[ALARMID].TaskID = TASKID; MyOS.Alarms[ALARMID].AlarmTicks = 0;

#if OS_TRACE_ON == 1
	#define OS_TRACE(X)\
				OS_Tracebuffer[OS_TraceIdx++] = X ; \
				if(OS_TraceIdx >= sizeof(OS_Tracebuffer)) OS_TraceIdx = 0;
#else
	#define OS_TRACE(X) ;
#endif

// *********  OS function prototypes

void 	OS_CustomISRCode(); // do not call; just fill in your code.

void 	OS_StartExecution(); // Start the operating system

void 	OS_SetEvent(uint8_t TaskID, uint8_t EventMask); // Set one or more events

uint8_t OS_WaitEvent(uint8_t EventMask); //returns event(s) in a mask, which lead to execution

void 	OS_MutexGet(int8_t mutexID); // number of mutexes limited to OS_NUMMUTEX !!!
				// Try to get a mutex; execution will block as long the mutex is occupied. If it is free, it is occupied afterwards.

void 	OS_MutexRelease(int8_t mutexID); // release the occupied mutex

void 	OS_SetAlarm(uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ); // set Alarm for the future and continue // set alarm to 0 disable it.

void 	OS_WaitAlarm(uint8_t AlarmID); // Wait for an Alarm set by OS_SetAlarm

uint8_t OS_QueueIn(OS_Queue_t* pQueue , uint8_t *pData); // Put byte into queue, return 1 if q full.

uint8_t OS_QueueOut(OS_Queue_t* pQueue, uint8_t *pData); // Get a byte out of the queue, return 1 if q empty.


void OS_TaskCreateInt( void (*t)(), uint8_t taskNum, uint8_t *stack, uint16_t stackSize ) ; // Create the task internal

#if OS_USEEXTCHECKS == 1
	void OS_ErrorHook(uint8_t ErrNo);
#else
	static inline void OS_ErrorHook(uint8_t ErrNo){};
#endif

#if OS_USEMEMCHECKS == 1
uint16_t OS_GetUnusedStack (uint8_t TaskID); // give the free stack space for any task as result.
uint8_t OS_GetQueueSpace(OS_Queue_t* pQueue); // give the free space in a queue
#endif

#if OS_USECLOCK == 1
void 	OS_GetTicks(uint32_t* pTime); // fills given variable with the OS ticks since start.
#endif

#if OS_DO_TESTSUITE == 1
void 	OS_TestSuite(void); // execute regression test of FabOS (OS development only)
#endif


// Wait for a certain number of OS-ticks (1 = wait to the next timer interrupt)

#if OS_USECOMBINED == 1
uint8_t OS_WaitEventTimeout(uint8_t EventMask, uint8_t AlarmID, OS_TypeAlarmTick_t numTicks ); //returns event on event, 0 on timeout.
#endif

#define OS_WaitTicks(AlarmID,numTicks) do{\
		OS_SetAlarm(AlarmID,numTicks);\
		OS_WaitAlarm(AlarmID);\
		}while(0)


// *********  CPU related assembler stuff

#define OS_DISABLEALLINTERRUPTS cli();
#define OS_ENABLEALLINTERRUPTS sei();

// Save all CPU registers on the AVR chip.
// The last two lines save the status register.
#define OS_INT_SAVE_CONTEXT() \
asm volatile( \
"	push r0\n\t\
	push r1\n\t\
	push r2\n\t\
	push r3\n\t\
	push r4\n\t\
	push r5\n\t\
	push r6\n\t\
	push r7\n\t\
	push r8\n\t\
	push r9\n\t\
	push r10\n\t\
	push r11\n\t\
	push r12\n\t\
	push r13\n\t\
	push r14\n\t\
	push r15\n\t\
	push r16\n\t\
	push r17\n\t\
	push r18\n\t\
	push r19\n\t\
	push r20\n\t\
	push r21\n\t\
	push r22\n\t\
	push r23\n\t\
	push r24\n\t\
	push r25\n\t\
	push r26\n\t\
	push r27\n\t\
	push r28\n\t\
	push r29\n\t\
	push r30\n\t\
	push r31\n\t\
	in r0,0x3f\n\t\
	push r0\n\t\
	clr r1\n\t");

// Restore all AVR CPU Registers. The first two lines
// restore the status register.
#define OS_INT_RESTORE_CONTEXT() \
asm volatile( \
"	pop r0\n\t\
	out 0x3f,r0\n\t\
	pop r31\n\t\
	pop r30\n\t\
	pop r29\n\t\
	pop r28\n\t\
	pop r27\n\t\
	pop r26\n\t\
	pop r25\n\t\
	pop r24\n\t\
	pop r23\n\t\
	pop r22\n\t\
	pop r21\n\t\
	pop r20\n\t\
	pop r19\n\t\
	pop r18\n\t\
	pop r17\n\t\
	pop r16\n\t\
	pop r15\n\t\
	pop r14\n\t\
	pop r13\n\t\
	pop r12\n\t\
	pop r11\n\t\
	pop r10\n\t\
	pop r9\n\t\
	pop r8\n\t\
	pop r7\n\t\
	pop r6\n\t\
	pop r5\n\t\
	pop r4\n\t\
	pop r3\n\t\
	pop r2\n\t\
	pop r1\n\t\
	pop r0\n\t");



// *********  some final warning calculations


#if (!defined(OS_NUMTASKS		))||\
	(!defined(OS_NUMMUTEX 		))||\
	(!defined(OS_NUMALARMS		))||\
	(!defined(OS_ScheduleISR 	))||\
	(!defined(OS_USECLOCK 		))||\
	(!defined(OS_USECOMBINED 	))||\
	(!defined(OS_USEEXTCHECKS   ))||\
	(!defined(OS_USEMEMCHECKS 	))||\
	(!defined(OS_UNUSEDMASK 	))||\
	(!defined(OS_TRACE_ON       ))||\
	(!defined(OS_TRACESIZE      ))
	#error not all defines in FabOS_config.h are done as described here (FabOS.h) below!
#endif


/* Example defines for FabOS_config.h     

#define OS_NUMTASKS  4 // Number of (OS_Create)Tasks ; never >8 (idle task is not counted here!)
#define OS_NUMMUTEX  3 // Number of Mutexes
#define OS_NUMALARMS 5 // Number of Alarms

#define OS_ScheduleISR TIMER1_COMPA_vect // Interrupt Vector used for OS-tick generation (check out CustomOS_ISRCode if you want to add isr code)

#define OS_USECLOCK 1 		// Use "OS_GetTicks()" which returns a 32bit timer tick
#define OS_USECOMBINED 1 	// Use "OS_WaitEventTimeout()" which is easier to use, than combining alarms and events to get the functionality.
#define OS_USEEXTCHECKS 1	// check wrong usage of OS API -> does not work, but no damage to OS stability.
#define OS_USEMEMCHECKS 1 	// Enable "OS_get_unused_Stack()" and "OS_GetQueueSpace()"
#define OS_UNUSEDMASK 0xEE  // unused Stack RAM will be filled with this byte, if OS_USEMEMCHECKS == 1.
#define OS_TRACE_ON  1 		// enable trace to OS_Tracebuffer[]
#define OS_TRACESIZE 1000	// size of OS_Tracebuffer[] (depending on memory left ;-)

*/


#if OS_NUMMUTEX > OS_NUMTASKS 
	#warning "more mutexes than tasks? are you serious with that concept?"
#endif

#if (OS_DO_TESTSUITE == 1) && (\
		(	OS_NUMTASKS 	!=5	) ||\
		(	OS_NUMMUTEX 	!=3	) ||\
		(   OS_NUMALARMS    !=6 ) ||\
		(	OS_USEEXTCHECKS !=1 ) ||\
		(	OS_USECLOCK 	!=1	) ||\
		(	OS_USEMEMCHECKS !=1	) ||\
		(	OS_USECOMBINED 	!=1	) \
		) 
		#error "please configure the defines for the testsuite as stated above!"
#endif

#if OS_USEMEMCHECKS == 0
	#undef UNUSEDMASK
#endif

#if OS_UNUSEDMASK+OS_NUMTASKS > 0xff
	#error please redefine OS_UNUSEDMASK to a smaller number!
#endif

#endif // FABOS_H
