#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "uart.h"

void user_process1()
{
	int v1 = 5;
	while( 1 ) {
		v1++;
	}
}

void user_process2()
{
	int v2 = -12;
	while( 1 ) {
		v2 -= 2;
	}
}

void user_process3()
{
	int v3 = 0;
	while( 1 ) {
		v3 += 5;
	}
}


void
kmain(void){
	sched_init();
	uart_init();
	uart_send_str("Bonjour\n");
	uart_send_str("Quel age avez vous ?\n");
	uart_send_str(">> ");
	char str[5];
	uart_receive_str(str, 5);
	uart_send_str("Vous avez :");
	uart_send_str(str);
	uart_send_str(" ans");
	/*
	create_process((func_t*) &user_process1);
	create_process((func_t*) &user_process2);
	create_process((func_t*) &user_process3);
	timer_init();
	// Activation des interruptions
	ENABLE_IRQ();
	*/	
	
	__asm("cps 0x10"); // switch CPU to USER mode
	
	while(1) {
		sys_yield();
	}
}
