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
#define PROCESS_DETAILS_MUSIC_PAUSE 4

#define EXECUTION_TIME_MAX 2500000 // 10 ms


typedef int (func_t) (void);

struct pcb_s * current_process;
uint32_t next_tick;

struct block {

    int block_size;
    int* first_page;
    struct block* next;
    struct block* previous;


};

struct pcb_s {

	int num;

    uint32_t regs[13];
    uint32_t sp;
    uint32_t lr_svc;
    uint32_t lr_user;
    uint32_t cpsr;
    int status;
    int return_code;
    int status_details;
    unsigned int * page_table;
    func_t * entry;
    uint64_t date_veille;
    struct pcb_s * next_pcb;
    struct block* first_empty_block;
    struct block* first_empty_block_heap;
    int** allocated_adresses;
    int allocated_adresses_size;
    
    //pour le CFS
	uint64_t execution_time;
	uint64_t arrival_time;
};


/**
 * @brief create_process
 * @param entry
 */
void create_process(func_t* entry);

/**
 * @brief start_current_process
 */
void start_current_process();

/**
 * @brief sched_init
 */
void sched_init();

/**
 * @brief elect
 */
void elect();

/**
 * @brief sys_yield
 */
void sys_yield();

/**
 * @brief sys_yieldto
 * @param dest
 */
void sys_yieldto(struct pcb_s* dest);

/**
 * @brief do_sys_yield
 * @param sp_param_base
 */
void do_sys_yield(uint32_t * sp_param_base);

/**
 * @brief do_sys_yieldto
 * @param sp_param_base
 */
void do_sys_yieldto(uint32_t * sp_param_base);

/**
 * @brief sys_exit
 * @param status
 * @return
 */
int sys_exit(int status);

/**
 * @brief do_sys_exit
 * @param sp_param_base
 */
void do_sys_exit(uint32_t * sp_param_base);

void sys_wait(int details_flag);

void do_sys_wait(uint32_t * sp_param_base);
#endif
