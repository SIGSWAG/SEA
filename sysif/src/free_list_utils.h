#ifndef FREE_LIST_UTILS_H
#define FREE_LIST_UTILS_H
#include "sched.h"

void* allocate_pages(int nbPages, struct block** block, int page_size );
void free_pages(int nbPages, void* first_page, struct block** block, int page_size);




#endif
