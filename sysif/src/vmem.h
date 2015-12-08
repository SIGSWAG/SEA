#ifndef VMEM_H
#define VMEM_H


#define PAGE_SIZE 4096 //4 kBytes
#define FIRST_LVL_TT_COUNT 4096 //12 bits
#define FIRST_LVL_TT_SIZE 16384 //4096*32/8 ---> 16384 Bytes
#define SECON_LVL_TT_COUNT 256 //8 bits
#define SECON_LVL_TT_SIZE 1024 // 256*32/8 ---> 1024 Bytes

#include <stdint.h>
#include "sched.h"

int init_kern_translation_table(void);
void vmem_init();
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

#endif

