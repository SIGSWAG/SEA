#include "sched.h"
#include "kheap.h"
#include "hw.h"

struct pcb_s* current_process;
struct pcb_s kmain_process;

extern unsigned int* sp_sauv;

#define SIZE_OF_PROCESS_STACK_OCTETS 10000

/* SYS_ACTION */

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
	__asm("mov lr, %0" : : "r"(current_process->lr_user));
}

int
sys_exit(int status)
{
	current_process->code_retour = status;
	__asm("mov r0, #7");
	__asm("swi #0");
	__asm("mov sp, %0" : : "r"(current_process->sp));
	__asm("mov lr, %0" : : "r"(current_process->lr_user));
	return status;
}

/* DO_SYS_ACTION */

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
		// on enregistre les registres courants
		old_process->registres[i] = sp_sauv[i];
		// puis on charge les registres du nouveau processus
		sp_sauv[i] = current_process->registres[i];
	}
	// on enregistre le CPSR et lr_svc de l'ancien processus
	__asm("mrs %0, SPSR" : "=r"(old_process->cpsr));
	// puis on restaure le CPSR du nouveau processus
	__asm("msr SPSR, %0" : : "r"(current_process->cpsr));
}


void
do_sys_exit()
{
	current_process->etat = TERMINATED;
	current_process->previous->next = current_process->next;
	current_process->next->previous = current_process->previous;
	struct pcb_s* to_delete = current_process;
	elect();
	kFree((void *)to_delete->sp, SIZE_OF_PROCESS_STACK_OCTETS);
	kFree((void *)to_delete, sizeof(struct pcb_s));
	// ici tout est propre mais il faut encore que l'on change de contexte
	// on passe donc au processus suivant
	int i;
	for(i=0 ; i<13 ; i++)
	{
		// puis on charge les registres du nouveau processus
		sp_sauv[i] = current_process->registres[i];
	}
	// puis on restaure le CPSR du nouveau processus
	__asm("msr SPSR, %0" : : "r"(current_process->cpsr));
}

/* Autre */

void
sched_init(void)
{
	kheap_init();
	current_process = &kmain_process;
	current_process->next = current_process;
	current_process->previous = current_process;
	current_process->etat = RUNNING;
	current_process->code_retour = -1;
}

void
create_process(func_t* entry)
{
	struct pcb_s* pcb_res = (struct pcb_s*) kAlloc(sizeof(struct pcb_s));
	struct pcb_s* pcb_res_next = current_process->next;
	
	current_process->next = pcb_res;
	pcb_res->previous = current_process;

	pcb_res->next = pcb_res_next;
	pcb_res_next->previous = pcb_res;

	// pcb_res->lr_user = entry;
	pcb_res->lr_user = (func_t*)(&start_current_process);
	pcb_res->fonction_process = entry;
	pcb_res->sp = (uint32_t*) (kAlloc(SIZE_OF_PROCESS_STACK_OCTETS) + sizeof(uint8_t)*SIZE_OF_PROCESS_STACK_OCTETS);
	pcb_res->code_retour = -1;
	pcb_res->etat = READY;
	pcb_res->cpsr = 0b101010000;
}

void
elect(void)
{
	// si le processus courant tournait, alors on l'arrete
	current_process->etat = (current_process->etat==RUNNING) ? READY : current_process->etat;
	if(current_process == current_process->next)
	{
		// il n'y a plus de processus à executer : on termine le kernel
		terminate_kernel();
	}
	else
	{
		current_process = current_process->next;
		current_process->etat = RUNNING;
	}
}

void
start_current_process(void)
{
	current_process->fonction_process();
	sys_exit(0);
}
