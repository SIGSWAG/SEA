#ifndef SCHED_H
#define SCHED_H

#include <stdint.h>

typedef int (func_t) (void)	;
typedef enum {READY, RUNNING, TERMINATED} status;
struct pcb_s
{
	uint32_t registres[13]; 
	func_t* lr_user;
	func_t* lr_svc;
	uint32_t* sp;
	uint32_t* cpsr;
	struct pcb_s* next;
	struct pcb_s* previous;
	status etat;
};

// lance dest en tant que prochain processus à executer
void sys_yieldto(struct pcb_s* dest);
// laisse l'ordonnanceur choisir le prochain processus à executer
void sys_yield();
// Termine le processus courant avec le code de retour status.
void sys_exit(int status);

void do_sys_yieldto(void);
void do_sys_yield(void);
void do_sys_exit();

void sched_init(void);
void create_process(func_t* entry);
void elect();

#endif
