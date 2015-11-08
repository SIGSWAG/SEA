#include "syscall.h"
#include "sched.h"
#include "util.h"
#include "asm_tools.h"
#include "hw.h"

unsigned int* sp_sauv = 0;

void __attribute__((naked))
irq_handler(void)
{
	__asm("stmfd sp!, {r0-r12, lr}");
	__asm("mov %0, sp" : "=r"(sp_sauv));

	// PC -= 4
	sp_sauv[13] -= 4;

	set_next_tick_default();
	ENABLE_TIMER_IRQ();
	ENABLE_IRQ();

	do_sys_yield();

	// ^ = on restaure aussi le spsr dans le cpsr
	__asm("ldmfd sp!, {r0-r12, pc}^");
}

void __attribute__((naked))
swi_handler(void)
{
	__asm("stmfd sp!, {r0-r12, lr}");
	
	int vector = 0;
	__asm("mov %0, r0" : "=r"(vector));
	__asm("mov %0, sp" : "=r"(sp_sauv));
	
	switch(vector){
		case 1: do_sys_reboot();  break;
		case 2: do_sys_nop(); 	  break;
		case 3: do_sys_settime(); break;
		case 4:	do_sys_gettime(); break;
		case 5:	do_sys_yieldto(); break;
		case 6:	do_sys_yield();   break;
		case 7:	do_sys_exit();    break;
		default: PANIC();
	}
	__asm("ldmfd sp!, {r0-r12, pc}^");
}

void
sys_reboot(void)
{
	__asm("mov r0, #1");
	__asm("swi #0");
}

void
sys_nop(void)
{
	__asm("mov r0, #2");
	__asm("swi #0");
}

void
sys_settime(uint64_t date_ms)
{
	__asm("mov r0, #3");
	__asm("mov r1, %0" : : "r"(date_ms >> 32));
	__asm("mov r2, %0" : : "r"(date_ms));
	__asm("swi #0");
}

uint64_t
sys_gettime(void)
{
	uint64_t date_ms_low = 0;
	uint64_t date_ms_high = 0;
	__asm("mov r0, #4");
	__asm("swi #0");
	__asm("mov %0, r1" : "=r"(date_ms_low));
	__asm("mov %0, r0" : "=r"(date_ms_high));
	return (date_ms_high << 32) | (date_ms_low & 0xFFFFFFFF);
}

void
do_sys_reboot(void)
{
	__asm("b 0x0000");
	/*const int PM_RSTC = 0x2010001c;
	const int PM_WDOG = 0x20100024;
	const int PM_PASSWORD = 0x5a000000;
	const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
	Set32(PM_WDOG, PM_PASSWORD | 1);
	Set32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
	while(1);*/
}

void
do_sys_nop(void)
{
	return;
}

void
do_sys_settime()
{
	uint64_t date_ms = get64(sp_sauv, 2, 1);
	set_date_ms(date_ms);
	return;
}

void
do_sys_gettime()
{
	uint64_t date_ms = get_date_ms();
	set64(sp_sauv, 1, date_ms);
}

uint64_t get64(unsigned int* stack, unsigned int low, unsigned int high)
{
	uint64_t stack_low = stack[low];
	uint64_t stack_high = stack[high];
	return (stack_high << 32) | stack_low;
}

void set64(unsigned int* stack, unsigned int registre, uint64_t value)
{
	uint64_t value_low = value & 0xFFFFFFFF;
	uint64_t value_high = value >> 32;
	stack[registre] = value_low;
	stack[registre - 1] = value_high;
}
