#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "uart.h"

void user_process1()
{
	uart_send_str("-----------user_process1\n");
	uart_send_str("Bonjour\n");
	uart_send_str("Quel age avez vous ?\n");
	uart_send_str(">> ");
	char str[5];
	uart_receive_str(str, 5);
	uart_send_str("Vous avez :");
	uart_send_str(str);
	uart_send_str(" ans\n");
}

void user_process2()
{
	uart_send_str("-----------user_process2\n");
}

void user_process3()
{
	uart_send_str("-----------user_process3\n");
}

void user_process4()
{
	uart_send_str("-----------user_process4\n");
}


void
kmain(void){
	sched_init();
	
	uart_init();


	create_process((func_t*) &user_process2);
	create_process((func_t*) &user_process3);
	create_process((func_t*) &user_process1);
	create_process((func_t*) &user_process4);
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
