﻿#include "vmem.h"
#include "kheap.h"
#include "sched.h"
#include "uart.h"

#define BEGIN_IO_MAPPED_MEM 0x20000000
#define END_IO_MAPPED_MEM 0x20FFFFFF
#define END_KERNEL_MEM 0x1000000


uint8_t * occupation_table;

unsigned int * table_kernel;


void start_mmu_C()
{
     register unsigned int control;

    __asm("mcr p15, 0, %[zero], c1, c0, 0" :: [zero]"r"(0)); // Disable cache
    __asm("mcr p15, 0, r0, c7, c7, 0"); // Invalidate cache (data and instructions) 
    
    invalidate_TLB();
    
    // Enable ARMv6 MMU features (disable sub-page AP)
    control = (1 << 23) | (1 << 15) | (1 << 4) | 1;
    
    // Write control register
    __asm volatile("mcr p15, 0, %[control], c1, c0, 0" :: [control]"r" (control));
}

void invalidate_TLB() 
{
	__asm("mcr p15, 0, r0, c8, c7, 0"); // Invalidate TLB entries
	
	// Invalidate the translation lookaside buffer (TLB)
	__asm volatile("mcr p15, 0, %[data], c8, c7, 0" :: [data]"r"(0));
}

void configure_mmu_kernel() {
	configure_mmu_C((unsigned int)table_kernel);
}

void configure_mmu_C(register unsigned int pt_addr)
{
    //total++; ?????
	
	// Translation table 0
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt_addr));
	
	// Translation table 1
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));
	
	// Use translation table 0 for everything
	__asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));
	
	/* Set Domain 0 ACL to "Manager", not enforcing memory permissions
	 * Evert mapped section/page is in domain 0
	 */
	__asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x3));
}

unsigned int * init_table_page() 
{
	/** On alloue la table des pages de niveau 1 alignée sur 16384 **/
    unsigned int * table1 = (unsigned int *) kAlloc_aligned(FIRST_LVL_TT_SIZE, 14);

    /** champs de bits **/
    unsigned int TABLE_2_NORMAL_PERIPH = 0b000000110110;
    unsigned int TABLE_2_NORMAL_MEM = 0b000001110010;
    unsigned int TABLE_1_NORMAL = 0b0000000001;

    unsigned int TABLE_2_BITSET;

    /** Pour chaque entrée de la table de niveau 1 **/
    for(unsigned int i = 0; i < FIRST_LVL_TT_COUNT; i++) {
        /** Faute si i n'est pas entre 200 et 20F ou entre 0 et la fin du kernel **/
        if( !(i >= 0x200 && i <= 0x20F) && !(i < (((unsigned int)&__kernel_heap_end__) >> 20)) )
        {
            table1[i] = 0x0;
            continue;
        }

        /** peripherique mappe en memoire **/
        if(i>=0x200 && i<=0x20F)
        {
            TABLE_2_BITSET = TABLE_2_NORMAL_PERIPH;
        }
        else
        {
            TABLE_2_BITSET = TABLE_2_NORMAL_MEM;
        }

        /** On alloue une table de niveau 2 alignée sur 1024 **/
        unsigned int * table2 = (unsigned int *) kAlloc_aligned(SECON_LVL_TT_SIZE, 10);

        /** Pour chaque entrée dans cette table **/
        for(unsigned int j = 0; j < SECON_LVL_TT_COUNT; j++){
            /** concat i|j|champ de bits **/
            table2[j] = (i<<20) | (j<<12) | TABLE_2_BITSET;
        }
        /** On place l'adresse de la table de niveau 2 dans la table de niveau 1 **/
         table1[i] = (unsigned int)table2 | TABLE_1_NORMAL; //table2|champ de bits
    }
    
    return table1;
}


int init_kern_translation_table(void)
{
    table_kernel = init_table_page();

    configure_mmu_C((unsigned int)table_kernel);
    
    return 0;
}

void init_occupation_table(void)
{

    occupation_table = (uint8_t *) kAlloc(OCCUPATION_TABLE_SIZE); //TODO allouer une table avec 1 bit/frame == bool

    for(int i=0; i<OCCUPATION_TABLE_SIZE; i++){
        occupation_table[i] = 0;
    }


}

void vmem_init()
{

    init_occupation_table();
    init_kern_translation_table();
    start_mmu_C();

}



void vmem_alloc_for_userland(struct pcb_s* process)
{





}











uint32_t vmem_translate(uint32_t va, struct pcb_s* process)
{
    uint32_t pa; /*The result*/

    /* 1st and 2nd table addresses */
    uint32_t table_base;
    uint32_t second_level_table;

    /* Indexes */
    uint32_t first_level_index;
    uint32_t second_level_index;
    uint32_t page_index;

    /* Descriptors */
    uint32_t first_level_descriptor;
    uint32_t *first_level_descriptor_address;
    uint32_t second_level_decriptor;
    uint32_t *second_level_decriptor_adress;

    if(process == 0)
    {
        __asm("mrc p15, 0, %[tb], c2, c0, 0":[tb]"=r"(table_base));
    } else
    {
       table_base = (uint32_t) process-> page_table;
    }

    table_base = table_base & 0xFFFFC000;

    /* Indexes */
    first_level_index = (va>>20);
    second_level_index = ((va<<12)>>24);
    page_index = (va & 0x00000FFF);

    /* First level descriptor */
    first_level_descriptor_address = (uint32_t*) (table_base|(first_level_index<<2));
    first_level_descriptor = *(first_level_descriptor_address);

    /* Translation fault */
    if(! (first_level_descriptor & 0x3))
    {
        return (uint32_t) FORBIDDEN_ADDRESS;
    }


    /* Second level descriptor */
    second_level_table = first_level_descriptor & 0xFFFFFC00;
    second_level_decriptor_adress = (uint32_t*) (second_level_table | (second_level_index << 2));
    second_level_decriptor = *((uint32_t*) second_level_decriptor_adress);

    /* Translation fault */
    if (!(second_level_decriptor & 0x3))
    {
        return (uint32_t) FORBIDDEN_ADDRESS;
    }

    /* Physical addreseap_init(s */
    pa = (second_level_decriptor & 0xFFFFF000) | page_index;

    return pa;

}




