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
	__asm("swi #0");
}

void
sys_yield(void)
{
	__asm("mov r0, #6");
	__asm("swi #0");
}

int
sys_exit(int status)
{
	current_process->code_retour = status;
	__asm("mov r0, #7");
	__asm("swi #0");
	return status;
}

/* DO_SYS_ACTION */

void
do_sys_yieldto(void)
{
	struct pcb_s* dest;
	dest = (struct pcb_s*) sp_sauv[1];

	// sauvegarde
	__asm("cps 0x1F"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user));
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("cps 0x13"); // Retour au mode SVC
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr));
	int i=0;
	for (; i < 13 ; i++)
	{
		current_process->registres[i] = sp_sauv[i];
	}
	current_process->lr_svc = (func_t*)sp_sauv[13];
	
	// permutation de proccess
	current_process = dest;
		
	// restauration
	__asm("cps 0x1F"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps 0x13"); // Retour au mode SVC
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	for (i = 0; i < 13 ; i++)
	{
		sp_sauv[i] = current_process->registres[i];
	}
	sp_sauv[13] = (unsigned int)current_process->lr_svc;
}

void do_sys_yield() 
{
	// sauvegarde
	__asm("cps 0x1F"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user));
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("cps 0x13"); // Retour au mode SVC
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr));
	int i=0;
	for (; i < 13 ; i++)
	{
		current_process->registres[i] = sp_sauv[i];
	}
	current_process->lr_svc = (func_t*)sp_sauv[13];
	
	// permutation de proccess
	elect();
		
	// restauration
	__asm("cps 0x1F"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps 0x13"); // Retour au mode SVC
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	for (i = 0; i < 13 ; i++)
	{
		sp_sauv[i] = current_process->registres[i];
	}
	sp_sauv[13] = (unsigned int)current_process->lr_svc;
}


void
do_sys_exit()
{
	current_process->etat = TERMINATED;
	current_process->previous->next = current_process->next;
	current_process->next->previous = current_process->previous;
	struct pcb_s* to_delete = current_process;
	elect();
	// int code_retour = to_delete->code_retour;
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
	// on restaure le CPSR du nouveau processus
	__asm("msr SPSR, %0" : : "r"(current_process->cpsr));
	// on restaure le CPSR du nouveau processus
	sp_sauv[13] = (unsigned int)current_process->lr_svc;


	__asm("cps 0x1F"); // Mode système => lr et sp sont ceux du user
	// restauration
	__asm("mov sp, %0" : : "r"(current_process->sp));
	__asm("mov lr, %0" : : "r"(current_process->lr_user));
	__asm("cps 0x13"); // Retour au mode SVC
	//__asm("push {%0}": : "r"(code_retour));

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
	pcb_res->lr_svc = (func_t*)(&start_current_process);
	pcb_res->fonction_process = entry;
	pcb_res->sp = (uint32_t*) (kAlloc(SIZE_OF_PROCESS_STACK_OCTETS) + SIZE_OF_PROCESS_STACK_OCTETS);
	pcb_res->code_retour = -1; 
	pcb_res->etat = READY;
	pcb_res->cpsr = 0x150; // user
}

void
elect(void)
{
	if(current_process->next->etat == RUNNING)
	{
		// il n'y a plus de processus à executer : on termine le kernel
		terminate_kernel();
	}
	else
	{
		// si le processus courant tournait, alors on l'arrete
		if(current_process->etat == RUNNING)
		{
			current_process->etat = READY;
		}
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
