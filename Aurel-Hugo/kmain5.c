#include "syscall.h"
#include "util.h"
#include "sched.h"

struct pcb_s *p1, *p2;

void
user_process_1()
{
	int v1=5;
	while(1)
	{
		v1++;
		sys_yieldto(p2);
	} 
}

void
user_process_2()
{
	int v2=-12;
	while(1)
	{
		v2-=2;
		sys_yieldto(p1);
	}
}

int
kmain( void )
{
	sched_init();

	p1 = create_process((func_t*) &user_process_1);
	p2 = create_process((func_t*) &user_process_2);

	__asm("cps 0x10"); // switch CPU to USER mode
	
	sys_yieldto(p1);
	
	PANIC();
	
	return 0;
}

/* Question 5-15
	* si une application peut choisir à qui elle passe la main, alors cette application
		peut garder le controle de la machine (en passant la main à un autre processus qu'elle controle).
		Intentionnellement ou pas ..
	* de plus si le processus à qui on passe la main n'est pas "prêt", c'est à dire qu'il à besoin d'une
		ressource qui n'est pas disponible pour le moment alors le système perdra du temps à essayer de 
		lui passer la main.
	* le système est mieux placé pour savoir quel processus servir en premier (pour une utilisation plus
		fluide de l'OS).
*/
