#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

struct pcb_s * current_process;

struct pcb_s {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t sp;
	uint32_t lr_svc;
	uint32_t lr_user;
	//uint32_t cpsr;
};

typedef int (func_t) (void);

struct pcb_s* create_process(func_t* entry);

void sched_init();

void sys_yieldto(struct pcb_s* dest);

void do_sys_yieldto(uint32_t * sp_param_base);

#endif
