#include "sched.h"
#include "kheap.h"

struct pcb_s* current_process;
struct pcb_s kmain_process;

extern unsigned int* sp_sauv;

void
sys_yieldto(struct pcb_s* dest)
{
	__asm("mov r0, #5");
	__asm("mov r1, %0" : : "r"(dest));
	__asm("mov %0, lr" : "=r"(current_process->lr_user));
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("swi #0");
	__asm("mov sp, %0" : : "r"(current_process->sp));
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); // current_process est maintenant "dest"
}

void
sys_yield(void)
{
	__asm("mov r0, #6");
	__asm("mov %0, lr" : "=r"(current_process->lr_user));
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("swi #0");
	__asm("mov sp, %0" : : "r"(current_process->sp));
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); // current_process est maintenant "dest"
}

void
do_sys_yieldto(void)
{
	struct pcb_s* dest;
	dest = (struct pcb_s*) sp_sauv[1];
	int i;
	for(i=0 ; i<13 ; i++)
	{
		// on enregistre les registres courant
		current_process->registres[i] = sp_sauv[i];
		// puis on charge les registres du nouveau processus
		sp_sauv[i] = dest->registres[i];
	}
	// on enregistre le CPSR de l'ancien processus
	__asm("mrs %0, SPSR" : "=r"(current_process->cpsr));
	// puis on restaure le CPSR du nouveau processus
	__asm("msr SPSR, %0" : : "r"(dest->cpsr));
	// permutation des processus
	current_process = dest;
}

void
do_sys_yield(void)
{
	struct pcb_s* old_process = current_process;
	// permutation des processus
	elect();
	int i;
	for(i=0 ; i<13 ; i++)
	{
		// on enregistre les registres courant
		old_process->registres[i] = sp_sauv[i];
		// puis on charge les registres du nouveau processus
		sp_sauv[i] = current_process->registres[i];
	}
	// on enregistre le CPSR de l'ancien processus
	__asm("mrs %0, SPSR" : "=r"(old_process->cpsr));
	// puis on restaure le CPSR du nouveau processus
	__asm("msr SPSR, %0" : : "r"(current_process->cpsr));
}

void
sched_init(void)
{
	kheap_init();
	current_process = &kmain_process;
	current_process->next = current_process;
}

void
create_process(func_t* entry)
{
	struct pcb_s* pcb_res = (struct pcb_s*) kAlloc(sizeof(struct pcb_s));
	pcb_res->lr_user = entry;
	pcb_res->sp = (uint32_t*) (kAlloc( 10000 ) + sizeof(uint8_t)*10000);
	struct pcb_s* pcb_res_next = current_process->next;
	current_process->next = pcb_res;
	pcb_res->next = pcb_res_next;
}

void
elect()
{
	current_process = current_process->next;
}