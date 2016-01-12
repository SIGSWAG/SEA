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
    while(1){
        char msg[2];
        // bloquant mais attente non active
        uart_receive_str(msg, 2);
        // detection du caractere reçu
        if(musique_est_arretee())
        {
            if(msg[0] == 'D')
            {
                // Down
                // Play
                musique_lecture();
            }
        }
        else if(get_mode_musique() == MUSIQUE_MODE_PLAYLIST)
        {
            switch(msg[0]){
                case 'L':
                    // Left
                    // Ralentir
                    diminuer_vitesse();
                    break;
                case 'R':
                    // Right
                    // Accelerer
                    augmenter_vitesse();
                    break;
                case 'U':
                    // Up
                    musique_stop();
                    break;
                case 'D':
                    // Down
                    // Pause
                    musique_pause();
                    break;
                case 'F':
                    // Forward
                    musique_suivante();
                    break;
                case 'B':
                    // Backward
                    musique_precedente();
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
                    // reinitialisation des periphs audio
                    musique_stop();
                    init_materiel();
                    musique_lecture();
                    break;
                case 'O':
                    // FistOpened
                    break;
                default:
    				break;
    		}
        }
        else{
			switch(msg[0]){
                case 'L':
                    // Left
                    // Ralentir
                    play_ponctuel(1);
                    break;
                case 'R':
                    // Right
                    play_ponctuel(2);
                    break;
                case 'U':
                    // Up
                    break;
                case 'D':
                    // Down
                    break;
                case 'F':
                    // Forward
                    play_ponctuel(3);
                    break;
                case 'B':
                    // Backward
                    play_ponctuel(4);
                    break;
                case '+':
                    // Circle clockwise
                    break;
                case '-':
                    // Circle counterclockwise
                    break;
                case 'I':
                    // FistClosed
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
#if IRQS_ACTIVEES
		timer_init();
		ENABLE_IRQ();		
#endif
	
	__asm("cps 0x10"); // switch CPU to USER mode

	while(1) {
		sys_yield();
	}
}
