#include "util.h"
#include "hw.h"
#include "asm_tools.h"
#include "syscall.h"
#include "sched.h"

void user_process_1()
{
	int v1=5;
	while(1)
	{
		v1++;
	}
}

void user_process_2()
{
	int v2=-12;
	while(1)
	{
		v2-=2;
	}
}

void user_process_3()
{
	int v3=0;
	while(1)
	{
		v3+=5;
	}
}

void kmain( void )
{
	sched_init();
	create_process((func_t*)&user_process_1);
	create_process((func_t*)&user_process_2);
	create_process((func_t*)&user_process_3);
	ENABLE_IRQ();
	timer_init();
	__asm("cps 0x10"); // switch CPU to USER mode
	// **********************************************************************
	while(1)
	{
		sys_yield();
	}
}


/** Question 7.4 **
	Registres différents en mode IRQ :
	R13_irq : SP
	R14_irq : LR
	SPSR_irq : ancien CPSR à restaurer
*/