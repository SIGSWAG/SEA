#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

typedef int (func_t) (void)	;
struct pcb_s
{
	uint32_t registres[13]; 
	func_t* lr_user;
	func_t* lr_svc;
	uint32_t* sp;
	uint32_t* cpsr;
};

void do_sys_yieldto(void);

void sys_yieldto(struct pcb_s* dest);

void sched_init(void);
struct pcb_s* create_process(func_t* entry);

#endif
