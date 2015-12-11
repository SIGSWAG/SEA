#ifndef VMEM_H
#define VMEM_H


#define PAGE_SIZE 4096 //4 kBytes
#define FIRST_LVL_TT_COUNT 4096 //12 bits
#define FIRST_LVL_TT_SIZE 16384 //4096*32/8 ---> 16384 Bytes
#define SECON_LVL_TT_COUNT 256 //8 bits
#define SECON_LVL_TT_SIZE 1024 // 256*32/8 ---> 1024 Bytes
#define OCCUPATION_TABLE_SIZE 65536 // 0x10000 (2¹⁶) * 8/8 ---> 65536 Bytes

/**
  Adressage physique dans la RAM : 0x2000 0000 - 0x1000 0000 (2²⁹ - 2²⁸ = 2²⁸) = 268435456
  Adressage logique : 0x1 0000 0000 (2³²)
  Taille d'une frame : 0x1000 (2¹²) = 4096
  Taille de la table d'occupation des frames : 0x10000 (2¹⁶) * 8/8 ---> 65536 Bytes
  **/

#include <stdint.h>
#include "sched.h"


int init_kern_translation_table(void);
void vmem_init();
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

unsigned int * init_table_page();

void invalidate_TLB();

#endif

