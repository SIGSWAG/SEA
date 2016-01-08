#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "uart.h"

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
				break;
			case 'U':
				// Up
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
		}
	}
}

void
kmain(void){
	sched_init();
	
	uart_init();
	
	create_process((func_t*) &serialReceiver);
	/*
	timer_init();
	// Activation des interruptions
	ENABLE_IRQ();
	*/	
	
	__asm("cps 0x10"); // switch CPU to USER mode
	
	while(1) {
		sys_yield();
	}
}
