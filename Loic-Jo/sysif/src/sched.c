#include "sched.h"

struct pcb_s * current_process;

struct pcb_s kmain_process;

static uint32_t * sp_param_base;

void sched_init()
{
	current_process = &kmain_process;
}

void sys_yieldto(struct pcb_s * dest)
{
	__asm("mov r0, #5");
	__asm("mov r1, %0" : : "r"(dest));
	__asm("SWI #0");	
	
}

void do_sys_yieldto()
{
	
	// save last process info
	
	// save context into the current_process struct
	
	*current_process = *((struct pcb_s *)(sp_param_base + 1));
	
	//retreive data from current_process struct into context
	
	*sp_param_base = *current_process->r0;
	*(sp_param_base+1) = *current_process->r1;
	*(sp_param_base+2) = *current_process->r2;
	*(sp_param_base+3) = *current_process->r3;
	*(sp_param_base+4) = *current_process->r4;
	*(sp_param_base+5) = *current_process->r5;
	*(sp_param_base+6) = *current_process->r6;
	*(sp_param_base+7) = *current_process->r7;
	*(sp_param_base+8) = *current_process->r8;
	*(sp_param_base+9) = *current_process->r9;
	*(sp_param_base+10) = *current_process->r10;
	*(sp_param_base+11) = *current_process->r11;
	*(sp_param_base+12) = *current_process->r12;
	*(sp_param_base+13) = *current_process->sp;
	*(sp_param_base+14) = *current_process->lr;
	
	
	/* NOPE
	__asm("ldr r0, [%0]" : : "r"(current_process->r0));  
	__asm("ldr r1, [%0]" : : "r"(current_process->r1));  
	__asm("ldr r2, [%0]" : : "r"(current_process->r2));  
	__asm("ldr r3, [%0]" : : "r"(current_process->r3));  
	__asm("ldr r4, [%0]" : : "r"(current_process->r4));  
	* */
}

