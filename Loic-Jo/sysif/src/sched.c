#include "sched.h"
#include "kheap.h"

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;


void sched_init()
{
	current_process = &kmain_process;
	kheap_init();
}

struct pcb_s* create_process(func_t* entry) 
{
	struct pcb_s * pcb = (struct pcb_s *) kAlloc(sizeof(struct pcb_s));
	pcb->lr_svc = (uint32_t) entry;
	pcb->lr_user = (uint32_t) entry;
	__asm("mrs %0, cpsr" : "=r"(pcb->cpsr));
	
	uint32_t * sp_zone = (uint32_t *) kAlloc(10000);
	pcb->sp = (uint32_t) sp_zone + 10000;
	
	return pcb;
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
	
	current_process->lr_svc = *(sp_param_base + 13);
	*(sp_param_base + 13) = dest->lr_svc;
	
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr)); // /** Bijour, ci moi la banane qui code :) **/
	
	 current_process = dest;
	 
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	
}

