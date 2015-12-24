#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>
// Process status
#define PROCESS_TERMINATED 0
#define PROCESS_RUNNING 1
#define PROCESS_WAITING 2
#define PROCESS_CREATED 3

typedef int (func_t) (void);

struct pcb_s * current_process;

struct block {

    int block_size;
    int* first_page;
    struct block* next;
    struct block* previous;


};
struct pcb_s {
    uint32_t regs[13];
    uint32_t sp;
    uint32_t lr_svc;
    uint32_t lr_user;
    uint32_t cpsr;
    int status;
    int returnCode;
    unsigned int * page_table;
    func_t * entry;
    struct pcb_s * next_pcb;
    struct block* first_empty_block;
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

#endif
