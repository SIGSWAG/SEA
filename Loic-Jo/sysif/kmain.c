#include "syscall.h"
#include "util.h"
#include "sched.h"

struct pcb_s pcb1, pcb2;

struct pcb_s *p1, *p2;

void
dummy() 
{
	return;
}

int
div(int dividend, int divisor)
{
	int result = 0;
	int remainder = dividend;
	
	while(remainder >= divisor) {
		result++;
		remainder -= divisor;
	}
	
	return result;
}

int
compute_volume(int rad)
{
	int rad3 = rad * rad * rad;
	
	return div(4*355*rad3, 3*113);
}

void
user_process_1()
{
	int v1=5;
	while(1){
		v1++;
		sys_yieldto(p2);
	}
}


void
user_process_2()
{
	int v2=-12;
	while(1){
		v2-=2;
		sys_yieldto(p1);
	}
}


void
kmain(void){	
	sched_init();
	
	p1 = &pcb1;
	p2 = &pcb2;
	
	//initialize p1 and p2
	
	//CODE
	
	
	__asm("cps 0x10"); // switch CPU to USER mode
	
	sys_yieldto(p1);
	
	// unreachable
	PANIC();
}


/*
int
kmain(void)
{	
	//__asm("cps 0x10");
	// SWI
	//sys_reboot();
	
	//while(1) {
	//	sys_nop();
	//}
	//sys_reboot();
	uint64_t date_ms = 0x12345678cacacaca;
	sys_settime(date_ms);
	
	// User
	//__asm("cps #19");
	//__asm("mov r14, r11");
	//__asm("mov cpsr, r12");
	// System
	//__asm("cps #31");  
	int radius = 5;
	int volume;
	
	dummy();
	volume = compute_volume(radius);
	
	return volume;
}
*/
