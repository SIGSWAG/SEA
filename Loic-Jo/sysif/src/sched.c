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
	pcb->lr_user = (uint32_t) entry;
	
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
	// save context into the current_process struct
	
	struct pcb_s * dest = ((struct pcb_s *)*(sp_param_base + 1));
	
	//retreive data from current_process struct into context
	
	current_process->r0 = *sp_param_base;
	*sp_param_base = dest->r0;
	
	current_process->r1 = *(sp_param_base+1);
	*(sp_param_base+1) = dest->r1;
	
	current_process->r2 = *(sp_param_base+2);
	*(sp_param_base+2) = dest->r2;
	
	current_process->r3 = *(sp_param_base+3);
	*(sp_param_base+3) = dest->r3;
	
	current_process->r4 = *(sp_param_base+4);
	*(sp_param_base+4) = dest->r4;
	
	current_process->r5 = *(sp_param_base+5);
	*(sp_param_base+5) = dest->r5;
	
	current_process->r6 = *(sp_param_base+6);
	*(sp_param_base+6) = dest->r6;
	
	current_process->r7 = *(sp_param_base+7);
	*(sp_param_base+7) = dest->r7;
	
	current_process->r8 = *(sp_param_base+8);
	*(sp_param_base+8) = dest->r8;
	
	current_process->r9 = *(sp_param_base+9);
	*(sp_param_base+9) = dest->r9;
	
	current_process->r10 = *(sp_param_base+10);
	*(sp_param_base+10) = dest->r10;
	
	current_process->r11 = *(sp_param_base+11);
	*(sp_param_base+11) = dest->r11;
	
	current_process->r12 = *(sp_param_base+12);
	*(sp_param_base+12) = dest->r12;
	
	 current_process = dest;
	
}

