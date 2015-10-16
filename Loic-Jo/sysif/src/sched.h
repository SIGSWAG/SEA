#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

struct pcb_s * current_process;

struct pcb_s {

	uint32_t regs[13];
	uint32_t sp;
	uint32_t lr_svc;
	uint32_t lr_user;
	uint32_t cpsr;
	struct pcb_s * next_pcb;
	int isTerminated;
	int returnCode;
};

typedef int (func_t) (void);

void create_process(func_t* entry);

void sched_init();

void elect();

void sys_yield();

void do_sys_yield();

void sys_yieldto(struct pcb_s* dest);

void do_sys_yieldto(uint32_t * sp_param_base);

int sys_exit();

void do_sys_exit();

#endif
