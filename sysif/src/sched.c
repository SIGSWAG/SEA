#include "sched.h"
#include "kheap.h"
#include "tree.h"
#include "hw.h"
#include "syscall.h"

#define SP_SIZE 10000

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;

tree cfs_tree;
/** En paramètre? **/
uint64_t change_time;

int nb_process = 0;

void sched_init()
{

	//Init a tree, with its nil node
	node* nil = (node*) kAlloc(sizeof(node));
	nil->color=BLACK;
	nil->left = nil;
	nil->right = nil;
	nil->parent = nil;
	cfs_tree.nil = nil;
	cfs_tree.root = nil;
	cfs_tree.nb_node = 0;
	nil->key = -1;
    
	// initialisation du process kmain
	kmain_process.status = PROCESS_RUNNING;
	current_process = &kmain_process;
	
	kmain_process.num=nb_process++;
	kmain_process.execution_time=0;
	insert_in_tree(&cfs_tree, 0, &kmain_process);

	//initialisation de la date à 0
	sys_settime(900);

	//initialisation du tas
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
	
	pcb->status = PROCESS_CREATED;
	
	// On initialise le temps d'execution à 0 et on enregistre la date d'arrivée du processus
	pcb->num=nb_process++;
	pcb->execution_time=0;
	pcb->arrival_time = sys_gettime();
	
	// On intègre le nouveau processus dans l'arbre du CFS
	insert_in_tree(&cfs_tree, 0, pcb);
	
	return;
}

void start_current_process()
{
	current_process->entry();
	sys_exit(0);
}

void elect() 
{

	//On détruit le processus s'il est fini, on le réinsère dans l'arbre sinon
	if(current_process->status == PROCESS_TERMINATED)
	{
		kFree((uint8_t*)current_process, sizeof(struct pcb_s));
	} else {
		insert_in_tree(&cfs_tree, current_process->execution_time, current_process);
	}
	
	/** Changement de processus **/

	//on cherche le processus qui a été exécuté le moins longtemps et on l'enlève de l'arbre
	node* current_node = tree_minimum(cfs_tree.nil, cfs_tree.root);
	rb_delete(&cfs_tree, current_node);
	
	//on récupère la pcb et on libère le noeud
	current_process = current_node->process;
	kFree((uint8_t*)current_node, sizeof(node));
	
	current_process->status = PROCESS_RUNNING;
	
}

void sys_yield() 
{
	__asm("mov r0, #6");
	__asm("SWI #0");
}

void do_sys_yield(uint32_t * sp_param_base) 
{
	
	//On met à jour le temps d'exécution
	current_process->execution_time += (get_date_ms() - change_time);

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
	
	//On enregistre la date de changement
	change_time=get_date_ms();
	
	//Le temps donné est égal au temps d'attente divisé par le nombre de processus en attente.
	next_tick = (change_time - current_process->execution_time - current_process->arrival_time);
	next_tick = divide(next_tick,cfs_tree.nb_node + 1);
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
	//Le processus est terminé
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

