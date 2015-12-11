#ifndef PMH_H
#define PMH_H

#define MAX_HEAP_MAX_SIZE 1024 + 1

// Tas-max utilisé pour trouver le processus le plus prioritaire en O(1).
// Son indice 0 n'est pas utilisé pour accélerer son parcours
typedef struct {
	struct pcb_s * heap[MAX_HEAP_MAX_SIZE];
	unsigned int last_used_index;
} process_max_heap;

void max_heap_init(process_max_heap * pmh, struct pcb_s * process_init);

void max_heap_add(process_max_heap * pmh, struct pcb_s * process_to_add);

struct pcb_s * max_heap_remove(process_max_heap * pmh);

void switch_indexes(process_max_heap * pmh, unsigned int index1, unsigned int index2);

unsigned int get_father_index(process_max_heap * pmh, unsigned int index);

unsigned int get_first_child_index(process_max_heap * pmh, unsigned int index);

unsigned int get_second_child_index(process_max_heap * pmh, unsigned int index);

#endif
