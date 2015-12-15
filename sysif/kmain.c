#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "uart.h"

void low_priority()
{
	int v1 = 0;
	while(++v1 < 5){
		uart_send_str("low_priority s'execute...\n");
		sys_yield();
	}
}

void high_priority_1()
{
	int v2 = 0;
	while(++v2 < 5){
		uart_send_str("high_priority_1 s'execute...\n");
		sys_yield();
	}
}

void high_priority_2()
{
	int v3 = 0;
	while(++v3 < 3){
		uart_send_str("high_priority_2 s'execute...\n");
		sys_yield();
	}
}

void
kmain(void)
{
	uart_init();
	
	sched_init();
	
	create_process((func_t*) &high_priority_1, 10);
	create_process((func_t*) &high_priority_2, 10);
	create_process((func_t*) &low_priority, 5);
	
	// Désactivation du timer pour tester l'ordonnanceur à priorité fixe	
	//timer_init();
	// Activation des interruptions
	//ENABLE_IRQ();
	
	__asm("cps 0x10"); // switch CPU to USER mode
	
	while(1) {
		sys_yield();
	}
	
}
