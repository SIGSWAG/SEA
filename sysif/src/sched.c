#include "sched.h"
#include "kheap.h"
#include "tree.h"
#include "hw.h"
#include "vmem.h"
#include "pwm.h"
#include "asm_tools.h"
#include "syscall.h"

#define SP_SIZE 10000

struct pcb_s kmain_process;

uint32_t lr_user;
uint32_t sp_user;

tree cfs_tree;

uint64_t change_time;
struct pcb_s sleeping_process;

int nb_process = 0;

void sched_init()
{
	//initialisation du tas
    kheap_init();
#if VMEM
    vmem_init();
#endif

    // initialisation du process kmain
    kmain_process.next_pcb = &kmain_process;
    kmain_process.status = PROCESS_RUNNING;
    kmain_process.page_table = init_table_page();
    current_process = &kmain_process;

#if CFS
	//Init a tree, with its nil node
	
	node* nil = (node*) kAlloc(sizeof(node));
	nil->color=BLACK;
	nil->left = nil;
	nil->right = nil;
	nil->parent = nil;
	nil->key = -1;
	cfs_tree.nil = nil;
	cfs_tree.root = nil;
	cfs_tree.nb_node = 0;
	
	kmain_process.num=nb_process++;
	kmain_process.execution_time=0;
	
	//initialisation de la liste des processus dormants
	sleeping_process.num=0;
	sleeping_process.next_pcb = &sleeping_process;

	//initialisation de la date à 0
	sys_settime(0);
#endif
}

void create_process(func_t* entry) 
{
    // On configure la MMU avec la table des pages système
    invalidate_TLB();
    configure_mmu_kernel();

    // Allocation de la place pour la pcb
    struct pcb_s * pcb = (struct pcb_s *) kAlloc(sizeof(struct pcb_s));

    // Allocation et initialisation du premier bloc (pour l'instant, rien n'est alloué par le processus)
    struct block * block = (struct block *) kAlloc(sizeof(struct block));
    block->block_size = 1044480;//2^32 / 4096 - 4096 (taille totale adressable - taille kernel space)
    block->first_page = (int*) 0x1000000; //début de la RAM user
    block->next = 0; //un seul bloc disponible
    block->previous = 0;

    // Mise en place du lr_svc, lr_user, et du cpsr
    pcb->entry = entry;
    pcb->lr_svc = (uint32_t)&start_current_process;
    pcb->lr_user = (uint32_t)pcb->entry;
    pcb->first_empty_block = block;
    pcb->cpsr = 0x150; //1010 10000 -> User mode + no interrupt
    //__asm("mrs %0, cpsr" : "=r"(pcb->cpsr));

    pcb->status = PROCESS_WAITING;
    pcb->status_details = PROCESS_DETAILS_NONE;

#if CFS
#else
    // On chaîne de manière circulaire current_process
    struct pcb_s * temp_pcb = current_process->next_pcb;
    current_process->next_pcb = pcb;
    pcb->next_pcb = temp_pcb;
#endif

    pcb->page_table = init_table_page();
    //Allocation de la stack, et on fait pointer sp tout en haut de ce qu'on vient allouer vu que SP décroit
    uint32_t * sp_zone = (uint32_t *) allocate_stack_for_process(pcb, 3);//(uint32_t *) kAlloc(SP_SIZE);
    pcb->sp = (uint32_t) sp_zone; //+ SP_SIZE;


    pcb->allocated_adresses = (int**) kAlloc(sizeof(int)*100);

    for(int i=0; i<100;i++){
        pcb->allocated_adresses[i]= (int*) kAlloc(sizeof(int)*2);

        pcb->allocated_adresses[i][0] = 0;
    }
    pcb->allocated_adresses_size = 100;

    //on reset la page des tables du processus courant
    invalidate_TLB();
    configure_mmu_kernel();
    
#if CFS    
    // On initialise le temps d'execution à 0 et on enregistre la date d'arrivée du processus
	pcb->num=nb_process++;
	pcb->execution_time=0;
	pcb->arrival_time = get_date_ms();
	
	// On intègre le nouveau processus dans l'arbre du CFS
	insert_in_tree(&cfs_tree, 0, pcb);
#endif

    return;
}

void __attribute__((naked)) start_current_process()
{
    __asm("blx lr");
    __asm("mov r0, %0" : : "r"(0));
    __asm("b sys_exit");

}

#if CFS
void update_process_list()
{
    struct pcb_s* process = &sleeping_process;
    while(process->next_pcb != &sleeping_process){
            switch(process->next_pcb->status_details){
            case PROCESS_DETAILS_WAITING_SERIAL:
                //check serial flag
                if ((Get32(UART_FR) & (1u << 4u)) == 0)
                {
                    process->next_pcb->status_details = PROCESS_DETAILS_NONE;
                    process->next_pcb->status = PROCESS_WAITING;
                    insert_in_tree(&cfs_tree, process->next_pcb->execution_time, process->next_pcb);
                    process->next_pcb = process->next_pcb->next_pcb;
		    process->next_pcb->next_pcb = NULL;
		    sleeping_process.num--;
                break;
            case PROCESS_DETAILS_WAITING_PWM_FIFO:
            {
                unsigned* pwm = (void*)PWM_BASE;
                long status = *(pwm + BCM2835_PWM_STATUS);
                if(!(status & BCM2835_FULL1)) // si la fifo de la musique n'est plus pleine
                {
                    process->next_pcb->status_details = PROCESS_DETAILS_NONE;
                    process->next_pcb->status = PROCESS_WAITING;
                    insert_in_tree(&cfs_tree, process->next_pcb->execution_time, process->next_pcb);
                    process->next_pcb = process->next_pcb->next_pcb;
		    process->next_pcb->next_pcb = NULL;
		    sleeping_process.num--;
                }
                break;
            }
            case PROCESS_DETAILS_WAITING_1_SECOND:
                if(process->next_pcb->date_veille + 10 <= get_date_ms()) // ~1.5s bizarre pour des ms..
                {
                    process->next_pcb->status_details = PROCESS_DETAILS_NONE;
                    process->next_pcb->status = PROCESS_WAITING;
                    insert_in_tree(&cfs_tree, process->next_pcb->execution_time, process->next_pcb);
                    process->next_pcb = process->next_pcb->next_pcb;
		    process->next_pcb->next_pcb = NULL;
		    sleeping_process.num--;
                }
                break;

            case PROCESS_DETAILS_MUSIC_PAUSE:
                if(musique_est_prete() == 0 || musique_est_arretee() == 0)
                {
                    process->next_pcb->status_details = PROCESS_DETAILS_NONE;
                    process->next_pcb->status = PROCESS_WAITING;
                    insert_in_tree(&cfs_tree, process->next_pcb->execution_time, process->next_pcb);
                    process->next_pcb = process->next_pcb->next_pcb;
		    process->next_pcb->next_pcb = NULL;
		    sleeping_process.num--;
                }
                break;
            default:
                break;
            }
        process = process->next_pcb;
    	}
	}
}
           

#else
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
            case PROCESS_DETAILS_WAITING_PWM_FIFO:
            {
                unsigned* pwm = (void*)PWM_BASE;
                long status = *(pwm + BCM2835_PWM_STATUS);
                if(!(status & BCM2835_FULL1)) // si la fifo de la musique n'est plus pleine
                {
                    process->status_details = PROCESS_DETAILS_NONE;
                    process->status = PROCESS_WAITING;
                }
                break;
            }
            case PROCESS_DETAILS_WAITING_1_SECOND:
                if(process->date_veille + 10 <= get_date_ms()) // ~1.5s bizarre pour des ms..
                {
                    process->status_details = PROCESS_DETAILS_NONE;
                    process->status = PROCESS_WAITING;
                }
                break;

            case PROCESS_DETAILS_MUSIC_PAUSE:
                if(musique_est_prete() == 0 || musique_est_arretee() == 0)
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
#endif

void elect() 
{

#if CFS
	//On détruit le processus s'il est fini, on le réinsère dans l'arbre sinon
	if(current_process->status == PROCESS_TERMINATED)
	{
		kFree((uint8_t*)current_process, sizeof(struct pcb_s));
	} else if(current_process->status == PROCESS_SLEEPING){
		current_process->next_pcb = sleeping_process.next_pcb;
		sleeping_process.next_pcb = current_process;
		sleeping_process.num++;
	} else {
		current_process->status = PROCESS_WAITING;
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
#else
	// TODO Voir si besoin dans le CFS
    update_process_list();

    // on kill tous les PROCESS_TERMINATED
    while(current_process->next_pcb->status != PROCESS_WAITING){
        if (current_process->next_pcb->status == PROCESS_TERMINATED)
        {
            struct pcb_s * process_to_kill = current_process->next_pcb;
            // on referme la chaine
            current_process->next_pcb = current_process->next_pcb->next_pcb;

            // kill next_process
            //libération de la mémoire du processus
            free_process_memory(process_to_kill);
            kFree((void *)process_to_kill, sizeof(struct pcb_s));

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
#endif
}

void sys_yield() 
{
    __asm("mov r0, #6");
    __asm("SWI #0");
}

void do_sys_yield(uint32_t * sp_param_base) 
{
#if CFS
	//On met à jour le temps d'exécution
	current_process->execution_time += (get_date_ms() - change_time);
#endif

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
	
#if CFS
	//On enregistre la date de changement
	change_time=get_date_ms();
	
	//Le temps donné est égal au temps d'attente divisé par le nombre de processus en attente.
	next_tick = (change_time - current_process->execution_time - current_process->arrival_time);
	next_tick = divide(next_tick,cfs_tree.nb_node + 1);
	if(next_tick > EXECUTION_TIME_MAX) next_tick = EXECUTION_TIME_MAX;
#endif
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
    //Le processus est terminé et récupération du statut
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
    __asm("mov r0, #12");
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
    current_process->date_veille = get_date_ms();

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


#ifdef IRQS_ACTIVEES
    set_next_tick_default();
    ENABLE_TIMER_IRQ();
    ENABLE_IRQ();
#endif
}

