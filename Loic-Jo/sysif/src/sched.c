#include "sched.h"
#include "kheap.h"
#include "hw.h"

#define SP_SIZE 10000

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;


void sched_init()
{
	current_process = &kmain_process;
	current_process->next_pcb = current_process;
	kheap_init();
}

void create_process(func_t* entry) 
{
	// Allocation de la place pour la pcb 
	struct pcb_s * pcb = (struct pcb_s *) kAlloc(sizeof(struct pcb_s));
	
	// Mise en place du lr_svc, lr_user, et du cpsr
	pcb->lr_svc = (uint32_t) entry;
	pcb->lr_user = (uint32_t) entry;
	__asm("mrs %0, cpsr" : "=r"(pcb->cpsr));
	
	// Allocation de la stack, et on fait pointer sp tout en haut de ce qu'on vient allouer vu que SP décroit
	uint32_t * sp_zone = (uint32_t *) kAlloc(SP_SIZE);
	pcb->sp = (uint32_t) sp_zone + SP_SIZE;
	
	pcb->isTerminated = 0;
	
	// On chaîne de manière circulaire current_process
	struct pcb_s * temp_pcb = current_process->next_pcb;
	current_process->next_pcb = pcb;
	pcb->next_pcb = temp_pcb;
	
	return;
}

void elect() 
{
	struct pcb_s * last_process = current_process;
	while(current_process->isTerminated) {
		last_process->next_pcb = current_process->next_pcb;
		current_process = current_process->next_pcb;
		if(current_process == last_process) {
			terminate_kernel();
		}
	}
	
	if(current_process == last_process) {
		current_process = current_process->next_pcb;
	}
}

void sys_yield() 
{
	__asm("mov r0, #6");
	__asm("SWI #0");
}

void do_sys_yield(uint32_t * sp_param_base) 
{
	// save lr_user and sp_user
	__asm("cps #31"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user)); 
	__asm("mov %0, sp" : "=r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// Sauvegarde de spsr dans current_process
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr));
	 
	// save context into the current_process struct
	int i;
	for (i = 0; i < 13 ; i++) {
		current_process->regs[i] = *(sp_param_base + i);
	}
	// Sauvegarde du LR_SVC
	current_process->lr_svc = *(sp_param_base + 13);
	
	
	//changement de proccess
	elect();
	
		
	// retrieve lr_user and sp_user
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// On met le cpsr du current process dans spsr
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	
	// retreive data from current_process struct into context
	for (i = 0; i < 13 ; i++) {
		*(sp_param_base + i) = current_process->regs[i];
	}
	// Restitution du LR_SVC
	*(sp_param_base + 13) = current_process->lr_svc;
}

int sys_exit(int status) 
{
	__asm("mov r0, #7");
	current_process->isTerminated = 1;
	current_process->returnCode = status;
	__asm("SWI #0");
	
	return status;
}

void do_sys_exit()
{	
	uint32_t sp = current_process->sp;
	// Libération de la zone mémoire allouée
	kFree((void*)&sp, SP_SIZE);
	kFree((void*)current_process, sizeof(struct pcb_s));
	
}

void sys_yieldto(struct pcb_s * dest)
{
	__asm("mov r0, #5");
	__asm("mov r1, %0" : : "r"(dest));
	
	__asm("SWI #0");
}

void do_sys_yieldto(uint32_t * sp_param_base)
{	
	__asm("cps #31"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user)); 
	__asm("mov %0, sp" : "=r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	// save context into the current_process struct
	
	struct pcb_s * dest = ((struct pcb_s *)*(sp_param_base + 1));
	
	//retreive data from current_process struct into context
	
	
	int i;
	//retreive data from current_process struct into context
	for (i = 0; i < 13 ; i++) {
		current_process->regs[i] = *(sp_param_base + i);
		*(sp_param_base + i) = dest->regs[i];
	}
	
	// Sauvegarde du LR_SVC
	current_process->lr_svc = *(sp_param_base + 13);
	*(sp_param_base + 13) = dest->lr_svc;
	
	// Sauvegarde de spsr dans current_process
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr)); 
	
	//changement de proccess
	 current_process = dest;
	 
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// On met le cpsr du current process dans spsr
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
}

