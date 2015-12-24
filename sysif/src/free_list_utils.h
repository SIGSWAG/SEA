#ifndef VMEM_H
#define VMEM_H
#include "sched.h"

void* allocate_pages(int nbPages, struct block* block );

void free_pages(int nbPages, void* first_page, struct block* block);




#endif
