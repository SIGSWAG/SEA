#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "pwm.h"

void
play_music()
{
//	audio_config();
	audio_test();
}

void
config_music()
{
	audio_config();
}

void
kmain(void)
{
	
	hw_init();
	sched_init();
	
	create_process((func_t*) &play_music);
	create_process((func_t*) &config_music);
	
	// Activation des interruptions
	// timer_init();
	// ENABLE_IRQ();
	
	__asm("cps 0x10"); // switch CPU to USER mode

	while(1) {
		sys_yield();
	}
}
