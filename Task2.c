/*
	FabOS example File

	(c) 2008-2010 Fabian Huslik

	Please change this file to your needs.
*/

#include "OS/FabOS.h"

// *********  User Task 1
void Task2() 
{

	OS_SetAlarm(1,50);

	while(1)
	{
		OS_WaitAlarm();
		OS_SetAlarm(1,50);
		
		// TODO add your code here



		
	}

}
