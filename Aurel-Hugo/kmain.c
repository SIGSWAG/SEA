#include "util.h"
#include "syscall.h"
#include "sched.h"
#define NB_PROCESS 5

void user_process(void)
{
	int v=0;
	for(;;)
	{
		v++;
		sys_yield();
	}
}

void user_process_stopping(void)
{
	int v=0;
	while(v<2)
	{
		v++;
		sys_yield();
	}
	sys_exit(0);
}

void kmain( void )
{
	sched_init();
	
	int i;
	for(i=0;i<NB_PROCESS;i++)
	{
		create_process((func_t*) &user_process_stopping);
	}

	__asm("cps 0x10"); // switch CPU to USER mode
	// **********************************************************************

	while(1)
	{
		sys_yield();
	}
}

/** Question 6.5 **
	Parce qu'il est impossible pour le moment de savoir quand le processus a fini son travail.
*/

/** Question 6.6 **
	On est obligé de faire un tour complet du round robin pour trouver le "previous" du pcb courant
	(celui à supprimer).
	Oui ça peut être génant si il y a trop de processus : perte de temps importante
*/