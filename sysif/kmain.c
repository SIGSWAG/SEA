#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"
#include "uart.h"
#include "pwm.h"

void serialReceiver()
{
	//	uart_send_str("-----------user_process1\n");
	while(1){
		char msg[2];
		// bloquant mais attente non active
		uart_receive_str(msg, 2);
		// detection du caractere reçu
		switch(msg[0]){
			case 'L':
				// Left
				break;
			case 'R':
				// Right
				// Accelerer
				augmenter_vitesse();
				break;
			case 'U':
				// Up
				// Ralentir
				diminuer_vitesse();
				break;
			case 'D':
				// Down
				break;
			case 'F':
				// Forward
				break;
			case 'B':
				// Backward
				break;
			case '+':
				// Circle clockwise
				// Monter le son
				augmenter_volume();
				break;
			case '-':
				// Circle counterclockwise
				// Baisser le son
				diminuer_volume();
				break;
			case 'I':
				// FistClosed
				// Play / pause
				if(musique_est_arretee()){
					musique_lecture();
				}
				else{
					musique_pause();
				}
				break;
			case 'O':
				// FistOpened
				break;
			default:
				break;
		}
	}
}






void user_process1(){

    int v1 = 5;
    /**int* alloc = (int*) sys_mmap();
    int* alloc2 = (int*) sys_mmap();
    alloc[0] = 1337;
    alloc2[0] = 7890;
    sys_munmap((void*)alloc);
    sys_munmap((void*)alloc2);*/

    /**alloc2 = (int*) sys_mmap();*/
    while( 1 ) {
        v1++;
    }

}



void
play_music()
{
	lance_audio();
}

void
config_music()
{
	configuration_audio();

}

void user_process2()
{

    int v2 = -12;
    while( 1 ){
        v2 -= 2;
    }
}

void user_process3()
{

	// Provoque un data abort
	//int * pt = (int *) 0x1100001;
	//*pt = 12;
	
    int* alloc = (int*) gmalloc(sizeof(int)*4);
    int* alloc2 = (int*) gmalloc(sizeof(int)*4);
    alloc[0] = 1337;
    alloc2[0] = 12345;


    gfree((void*)alloc);
    alloc = (int*) gmalloc(4096);


    int v3 = 0;
    while( 1 ) {
        v3 += 5;
    }
}

void print_boot_message() {
	uart_send_str(" _       __________    __________  __  _________ \n| |     / / ____/ /   / ____/ __ \\/  |/  / ____/ \n| | /| / / __/ / /   / /   / / / / /|_/ / __/ \n| |/ |/ / /___/ /___/ /___/ /_/ / /  / / /___  \n|__/|__/_____/_____/\\____/\\____/_/  /_/_____/ \n\n");
    uart_send_str("ssOS booting ...\nLoading the greatness ...\nPowered by SIGSWAG Ltd. Check us out on www.sigswag.com !\n");
    uart_send_str("              ____  _____ \n   __________/ __ \\/ ___/\n  / ___/ ___/ / / /\\__ \\ \n (__  |__  ) /_/ /___/ / \n/____/____/\\____//____/ \n\n"); 
                                                                  
}


void
kmain(void){

    /** Exemple de sortie console **/
    uart_init();
    print_boot_message();
    hw_init();
    sched_init();

    create_process((func_t*) &user_process1);
    //create_process((func_t*) &user_process2);
    create_process((func_t*) &user_process3);

    timer_init();
    // Activation des interruptions
    ENABLE_IRQ();

    __asm("cps 0x10"); // switch CPU to USER mode




    //uint32_t  tr = vmem_translate(0x8000, 0);



    while(1) {
#if CFS
		volatile int i;
		i=0;
		i++;
#else
        sys_yield();
#endif
        
    }
	//	uart_send_str("-----------user_process1\n");

}



