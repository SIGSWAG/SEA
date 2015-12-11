#include "pmh.h"
#include "sched.h"

// Initialise le tas-max avec une première valeur.
void max_heap_init(process_max_heap * pmh, struct pcb_s * process_init)
{
	// Case 0 ignorée pour accélerer les calculs
	pmh->heap[1] = process_init;
	pmh->last_used_index = 1;
}

// Ajoute un élément au tas-max.
void max_heap_add(process_max_heap * pmh, struct pcb_s * process_to_add)
{
	pmh->last_used_index++;
	pmh->heap[pmh->last_used_index] = process_to_add;
	
	// Percolations vers le haut
	unsigned int process_index = pmh->last_used_index;
	unsigned int father_index = get_father_index(pmh, process_index);
	while(pmh->heap[father_index]->priority < process_to_add->priority){
		switch_indexes(pmh, father_index, process_index);
		process_index = father_index;
		father_index = get_father_index(pmh, process_index);
	}
}

// Enlève le premier élément du tas-max.
// Retourne la pcb supprimée.
struct pcb_s * max_heap_remove(process_max_heap * pmh)
{
	pmh->last_used_index--;
	switch_indexes(pmh, 1, pmh->last_used_index);
	
	// Percolations vers le bas.
	unsigned int process_index = 1;
	unsigned int child1_index, child2_index, greatest_index;
	for(;;) {
		child1_index = get_first_child_index(pmh, process_index);
		child2_index = get_second_child_index(pmh, process_index);
		
		if(pmh->heap[child1_index]->priority > pmh->heap[child2_index]->priority)
			greatest_index = child1_index;
		else
			greatest_index = child2_index;
		
		if(pmh->heap[greatest_index]->priority > pmh->heap[process_index]->priority){
			switch_indexes(pmh, greatest_index, process_index);
			process_index = greatest_index;
		} else
			break;
	}
	
	return pmh->heap[pmh->last_used_index + 1];
}

// Inverse le contenu de deux cases du tas-max.
void switch_indexes(process_max_heap * pmh, unsigned int index1, unsigned int index2)
{
	struct pcb_s * temp = pmh->heap[index1];
	pmh->heap[index1] = pmh->heap[index2];
	pmh->heap[index2] = temp;
}

// Obtient l'indice du père d'un élément.
unsigned int get_father_index(process_max_heap * pmh, unsigned int index)
{
	return index >> 1;
}

// Obtient l'indice du premier enfant d'un élément.
// Retourne -1 s'il n'existe pas.
unsigned int get_first_child_index(process_max_heap * pmh, unsigned int index)
{
	return index << 1;
}

// Obtiens l'indice du second enfant d'un élément.
// Retourne -1 s'il n'existe pas.
unsigned int get_second_child_index(process_max_heap * pmh, unsigned int index)
{
	return (index << 1) + 1;
}
