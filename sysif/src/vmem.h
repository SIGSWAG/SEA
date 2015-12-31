#ifndef VMEM_H
#define VMEM_H


#define PAGE_SIZE 4096 //4 kBytes
#define FIRST_LVL_TT_COUNT 4096 //12 bits
#define FIRST_LVL_TT_SIZE 16384 //4096*32/8 ---> 16384 Bytes
#define SECON_LVL_TT_COUNT 256 //8 bits
#define SECON_LVL_TT_SIZE 1024 // 256*32/8 ---> 1024 Bytes
#define OCCUPATION_TABLE_SIZE 65536 // 0x10000 (2¹⁶) * 8/8 ---> 65536 Bytes

#define PROCESS_TABLE_INIT 0
#define KERNEL_TABLE_INIT 1

/**
  Adressage physique dans la RAM : 0x2000 0000 - 0x1000 0000 (2²⁹ - 2²⁸ = 2²⁸) = 268435456
  Adressage logique : 0x1 0000 0000 (2³²)
  Taille d'une frame : 0x1000 (2¹²) = 4096
  Taille de la table d'occupation des frames : 0x10000 (2¹⁶) * 8/8 ---> 65536 Bytes
  **/

#include <stdint.h>
#include "sched.h"


/**
 * @brief vmem_init initialie la mémoire virtuelle
 */
void vmem_init();

/**
 * @brief vmem_translate simule la MMU, test-purpose.
 * @param va
 * @param process
 * @return
 */
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

/**
 * @brief alloue une table des pages et l'initialise, puis renvoie un pointeur dessus
 * @return
 */
unsigned int * init_table_page();

void invalidate_TLB();
/**
 * @brief configure_mmu_kernel fait pointer TTBR0 sur la table des pages kernel
 */
void configure_mmu_kernel();

/**
 * @brief configure_mmu_C met à jour TTBR0 pour pointer sur pt_addr
 * @param pt_addr
 */
void configure_mmu_C(register unsigned int pt_addr);

/**
 * @brief vmem_alloc_for_userland permet d'allouer nbPages dans l'espace d'adressage de process
 * s'il n'y a pas assez de place, retourne 0. Sinon, renvoie un pointeur sur la première page
 * @param process
 * @param nbPages
 * @return
 */
void* vmem_alloc_for_userland(struct pcb_s* process, int nbPages);

/**
 * @brief vmem_desalloc_for_userland permet de désallouer nbPages commençant à page.
 * Le comportement de cette méthode n'est garanti que si nbPages ont été allouées a partir de page par vmem_alloc_for_userland
 * @param process
 * @param page
 * @param nbPages
 */
void vmem_desalloc_for_userland(struct pcb_s* process, void* page, int nbPages);


/**
 * @brief free_process_memory permet de libérer la mémoire d'un processus. (pile, tas, table des pages)
 * @param process
 */
void free_process_memory(struct pcb_s* process);

/**
 * @brief allocate_stack_for_process alloue nbPages dans les adresses hautes de la mémoire de process.
 * Cet méthode devrait être appelée avant toute autre allocation pour process.
 * @param process
 * @param nbPages
 * @return
 */
void* allocate_stack_for_process(struct pcb_s* process, int nbPages);


void* do_gmalloc(struct pcb_s* process, int size);

void do_gfree(struct pcb_s* process, void* pointer, int size);

#endif

