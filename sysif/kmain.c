#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "pwm.h"

void
play_music()
{
	lance_audio();
}

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

void
kmain(void)
{
	
	hw_init();
	sched_init();
	
	create_process((func_t*) &play_music);
	create_process((func_t*) &serialReceiver);
	
	// Activation des interruptions
#ifdef IRQS_ACTIVEES
		timer_init();
		ENABLE_IRQ();		
#endif
	
	__asm("cps 0x10"); // switch CPU to USER mode

	while(1) {
		sys_yield();
	}
}
