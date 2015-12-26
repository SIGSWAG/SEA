#include "sched.h"
#include "kheap.h"
#include "hw.h"
#include "asm_tools.h"

#define SP_SIZE 10000

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;


void sched_init()
{
	// initialisation du process kmain
	kmain_process.next_pcb = &kmain_process;
	kmain_process.status = PROCESS_RUNNING;
	current_process = &kmain_process;

	kheap_init();
}

void create_process(func_t* entry) 
{
	// Allocation de la place pour la pcb 
	struct pcb_s * pcb = (struct pcb_s *) kAlloc(sizeof(struct pcb_s));
	
	// Mise en place du lr_svc, lr_user, et du cpsr
	pcb->lr_svc = (uint32_t)&start_current_process;
	pcb->lr_user = (uint32_t)&start_current_process;
	pcb->entry = entry;
	pcb->cpsr = 0x150; //1010 10000 -> User mode + no interrupt
	//__asm("mrs %0, cpsr" : "=r"(pcb->cpsr));
	
	// Allocation de la stack, et on fait pointer sp tout en haut de ce qu'on vient allouer vu que SP décroit
	uint32_t * sp_zone = (uint32_t *) kAlloc(SP_SIZE);
	pcb->sp = (uint32_t) sp_zone + SP_SIZE;
	
	pcb->status = PROCESS_WAITING;
	pcb->status_details = PROCESS_DETAILS_NONE;
	
	// On chaîne de manière circulaire current_process
	struct pcb_s * temp_pcb = current_process->next_pcb;
	current_process->next_pcb = pcb;
	pcb->next_pcb = temp_pcb;
	
	return;
}

void start_current_process()
{
	current_process->entry();
	sys_exit(0);
}

// parcours les process pour voir s'ils sont endormi et s'il peut les reveiller
void update_process_list()
{
	struct pcb_s* process = current_process;
	do{
		if(process->status == PROCESS_SLEEPING){
			switch(process->status_details){
				case PROCESS_DETAILS_WAITING_SERIAL:
					//check serial flag
					if ((Get32(UART_FR) & (1u << 4u)) == 0)
					{
						process->status_details = PROCESS_DETAILS_NONE;
						process->status = PROCESS_WAITING;
					}
					break;
				default:
					break;
			}
		}
		process = process->next_pcb;
	}
	while(process != current_process);
}

void elect() 
{
	update_process_list();

	// on kill tous les PROCESS_TERMINATED
	while(current_process->next_pcb->status != PROCESS_WAITING){
		if (current_process->next_pcb->status == PROCESS_TERMINATED)
		{
			struct pcb_s * process_to_kill = current_process->next_pcb;
			// on referme la chaine
			current_process->next_pcb = current_process->next_pcb->next_pcb;
			
			// kill next_process
			// on considère (voir implentation de kalloc/kFree) que la mémoire est organisée telle que pcb et la stack sp sont contigus.
			// de plus pcb doit être situé en dessous de la stack. On libère alors la taille de la structure et de sa stack d'un seul coup.
			kFree((void *)process_to_kill, sizeof(struct pcb_s) + SP_SIZE);

			// S'il ne reste qu'un process dans la boucle (main)
			if(current_process->next_pcb == current_process){
				terminate_kernel();
			}
		}
		else{
			current_process = current_process->next_pcb;
		}
	}

	current_process = current_process->next_pcb;
	/*
	if(current_process->status == CREATED){
		// First time the process run
		// call start_current_process somehow
	}
	*/
	// 
	current_process->status = PROCESS_RUNNING;
	
}

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
	current_process->status = PROCESS_WAITING;
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
	__asm("mov r1, %0" : : "r"(status));
	__asm("SWI #0");
	
	return status;
}

void do_sys_exit(uint32_t * sp_param_base)
{	
	//récupération du statut
	current_process->returnCode = *(sp_param_base + 1);
	current_process->status = PROCESS_TERMINATED;
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

void sys_wait(int details_flag)
{
	__asm("mov r0, #8");
	__asm("mov r1, %0" : : "r"(details_flag));
	
	__asm("SWI #0");
}

void do_sys_wait(uint32_t * sp_param_base)
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
	// on endort le process
	current_process->status = PROCESS_SLEEPING;
	// r1 passé en paramètre
	current_process->status_details = current_process->regs[1];

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