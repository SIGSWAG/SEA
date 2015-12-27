#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

// Process status
#define PROCESS_TERMINATED 0
#define PROCESS_RUNNING 1
#define PROCESS_WAITING 2
#define PROCESS_CREATED 3
#define PROCESS_SLEEPING 4

#define PROCESS_DETAILS_NONE 0
#define PROCESS_DETAILS_WAITING_SERIAL 1
#define PROCESS_DETAILS_WAITING_PWM_FIFO 2
#define PROCESS_DETAILS_WAITING_1_SECOND 3


typedef int (func_t) (void);

struct pcb_s * current_process;

struct pcb_s {
	uint32_t regs[13];
	uint32_t sp;
	uint32_t lr_svc;
	uint32_t lr_user;
	uint32_t cpsr;
	int status;
	int return_code;
	int status_details;
	uint64_t date_veille;

	func_t * entry;
	struct pcb_s * next_pcb;
};

void create_process(func_t* entry);

void start_current_process();

void sched_init();

void elect();

void sys_yield();

void do_sys_yield();

void sys_yieldto(struct pcb_s* dest);

void do_sys_yieldto(uint32_t * sp_param_base);

int sys_exit(int status);

void do_sys_exit(uint32_t * sp_param_base);

void sys_wait(int details_flag);

void do_sys_wait(uint32_t * sp_param_base);
#endif
