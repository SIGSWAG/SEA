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
};

typedef int (func_t) (void);

struct pcb_s* create_process(func_t* entry);

void sched_init();

void sys_yieldto(struct pcb_s* dest);

void do_sys_yieldto(uint32_t * sp_param_base);

#endif
