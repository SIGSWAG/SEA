#include "sched.h"
#include "kheap.h"
#include "hw.h"
#include "pmh.h"

#define SCHEDULER PMH

#define SP_SIZE 10000

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;

#if SCHEDULER==PMH
process_max_heap pmh;
#endif

void sched_init()
{
	// initialisation du process kmain
	kmain_process.next_pcb = &kmain_process;
	kmain_process.priority = 0;
	kmain_process.status = PROCESS_RUNNING;
	current_process = &kmain_process;
	
#if SCHEDULER==PMH
	// Mise en place du tas-max
	max_heap_init(&pmh, &kmain_process);
#endif

	kheap_init();
}

void create_process(func_t* entry, int priority) 
{
	// Allocation de la place pour la pcb 
	struct pcb_s * pcb = (struct pcb_s *) kAlloc(sizeof(struct pcb_s));
	
	// Mise en place du lr_svc, lr_user, et du cpsr
	pcb->lr_svc = (uint32_t)&start_current_process;
	pcb->lr_user = (uint32_t)&start_current_process;
	pcb->entry = entry;
	pcb->priority = priority;
	pcb->cpsr = 0x150; //1010 10000 -> User mode + no interrupt
	//__asm("mrs %0, cpsr" : "=r"(pcb->cpsr));
	
	// Allocation de la stack, et on fait pointer sp tout en haut de ce qu'on vient allouer vu que SP décroit
	uint32_t * sp_zone = (uint32_t *) kAlloc(SP_SIZE);
	pcb->sp = (uint32_t) sp_zone + SP_SIZE;
	
	pcb->status = PROCESS_CREATED;
	
	// On chaîne de manière circulaire current_process
	struct pcb_s * temp_pcb = current_process->next_pcb;
	current_process->next_pcb = pcb;
	pcb->next_pcb = temp_pcb;
	
	// On ajoute le processus au tas-max
	max_heap_add(&pmh, pcb);
	
	return;
}

void start_current_process()
{
	current_process->entry();
	sys_exit(0);
}

#if SCHEDULER==PMH
void elect()
{
	if(current_process->status == PROCESS_TERMINATED){
		// Le processus en train de tourner est forcément le premier.
		max_heap_remove(&pmh);
		kFree((void *) current_process, sizeof(struct pcb_s) + SP_SIZE);
		
		// S'il ne reste qu'un processus, on a tout fini !
		if(pmh.last_used_index == 1){
			terminate_kernel();
			return ;
		}
	} else {
		current_process->status = PROCESS_WAITING;
	}
	
	current_process = pmh.heap[1];
	current_process->status = PROCESS_RUNNING;
}
#elif
// Implémentation naïve de recherche de priorité max par parcours des processus
void elect() 
{
	// Principe de l'ordonnanceur à priorité fixe :
	// On recherche le processus ayant la priorité la plus forte (en tuant les processus terminés au passage)
	// Si celui-ci est le processus courant, on reste desssus
	// Sinon, on bascule sur celui-ci
	// Si plusieurs processus ont la priorité la plus forte, on change sur un autre processus ayant cette priorité
	struct pcb_s * process = current_process;
	struct pcb_s * elected_process = current_process;
	while(process->next_pcb != current_process){
		
		if(process->next_pcb->status == PROCESS_TERMINATED){
			// Si le processus est terminé, on le tue
			
			struct pcb_s * process_to_kill = process->next_pcb;
			process->next_pcb = process->next_pcb->next_pcb; // Fermeture de la chaîne
			// On considère (voir implentation de kAlloc/kFree) que la mémoire est organisée telle que la pcb et la stack sp sont contigus.
			// De plus, pcb doit être situé en dessous de la stack. On libère alors la taille de la structure et de sa stack d'un seul coup.
			kFree((void *)process_to_kill, sizeof(struct pcb_s) + SP_SIZE);
			
		}
		
		// TODO peut-être revoir la logique, c'est un peu magouilleux mais ça a l'air de marcher, à tester plus en profondeur
		if(elected_process->status == PROCESS_TERMINATED || process->next_pcb->priority >= elected_process->priority) {
			// Si le processus suivant a une priorité au moins aussi haute que la priorité la plus haute jusqu'alors trouvée, on élit ce processus
			
			elected_process = process->next_pcb;
			
		}
		
		// Continuation du parcours de la liste chaînée circulaire...
		process = process->next_pcb;
		
	}
	
	// S'il ne reste qu'un processus, alors c'est kmain : le programme est terminé...
	if(elected_process->next_pcb == elected_process){
		terminate_kernel();
	}
	
	// Mise à jour des status
	if(current_process->status != PROCESS_TERMINATED){
		current_process->status = PROCESS_WAITING;
	}
	current_process = elected_process;
	current_process->status = PROCESS_RUNNING;
	
}
#endif

void sys_yield() 
{
	__asm("mov r0, #6");
	__asm("SWI #0");
}

void do_sys_yield(uint32_t * sp_param_base) 
{
	// save lr_user and sp_user
	__asm("cps #31"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user)); 
	__asm("mov %0, sp" : "=r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// Sauvegarde de spsr dans current_process
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr));
	 
	// save context into the current_process struct
	int i;
	for (i = 0; i < 13 ; i++) {
		current_process->regs[i] = *(sp_param_base + i);
	}
	// Sauvegarde du LR_SVC
	current_process->lr_svc = *(sp_param_base + 13);
	
	
	//changement de proccess
	elect();
	
		
	// retrieve lr_user and sp_user
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// On met le cpsr du current process dans spsr
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	
	// retreive data from current_process struct into context
	for (i = 0; i < 13 ; i++) {
		*(sp_param_base + i) = current_process->regs[i];
	}
	// Restitution du LR_SVC
	*(sp_param_base + 13) = current_process->lr_svc;
}

int sys_exit(int status) 
{
	__asm("mov r0, #7");
	current_process->status = PROCESS_TERMINATED;
	current_process->returnCode = status;
	__asm("SWI #0");
	
	return status;
}

void do_sys_exit(uint32_t * sp_param_base)
{	
	// Changement de process
	elect();

	// retrieve lr_user and sp_user
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// On met le cpsr du current process dans spsr
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
	
	int i = 0;
	// retreive data from current_process struct into context
	for (i = 0; i < 13 ; i++) {
		*(sp_param_base + i) = current_process->regs[i];
	}
	// Restitution du LR_SVC
	*(sp_param_base + 13) = current_process->lr_svc;
}

void sys_yieldto(struct pcb_s * dest)
{
	__asm("mov r0, #5");
	__asm("mov r1, %0" : : "r"(dest));
	
	__asm("SWI #0");
}

void do_sys_yieldto(uint32_t * sp_param_base)
{	
	__asm("cps #31"); // Mode système
	__asm("mov %0, lr" : "=r"(current_process->lr_user)); 
	__asm("mov %0, sp" : "=r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	// save context into the current_process struct
	
	struct pcb_s * dest = ((struct pcb_s *)*(sp_param_base + 1));
	
	//retreive data from current_process struct into context
	
	
	int i;
	//retreive data from current_process struct into context
	for (i = 0; i < 13 ; i++) {
		current_process->regs[i] = *(sp_param_base + i);
		*(sp_param_base + i) = dest->regs[i];
	}
	
	// Sauvegarde du LR_SVC
	current_process->lr_svc = *(sp_param_base + 13);
	*(sp_param_base + 13) = dest->lr_svc;
	
	// Sauvegarde de spsr dans current_process
	__asm("mrs %0, spsr" : "=r"(current_process->cpsr)); 
	
	//changement de proccess
	 current_process = dest;
	 
	__asm("cps #31"); // Mode système
	__asm("mov lr, %0" : : "r"(current_process->lr_user)); 
	__asm("mov sp, %0" : : "r"(current_process->sp));  
	__asm("cps #19"); // Retour au mode SVC
	
	// On met le cpsr du current process dans spsr
	__asm("msr spsr, %0" : : "r"(current_process->cpsr));
}
