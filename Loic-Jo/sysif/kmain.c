#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "pwm.h"

void
play_music()
{
	audio_test();
}

void
kmain(void)
{
	sched_init();
	
	create_process((func_t*) &play_music);
		
	timer_init();
	// Activation des interruptions
	ENABLE_IRQ();
	
	__asm("cps 0x10"); // switch CPU to USER mode
	
	while(1) {
		sys_yield();
	}
}
