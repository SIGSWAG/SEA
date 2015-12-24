#include "syscall.h"
#include "util.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"
#include "uart.h"


void user_process1()
{
    int v1 = 5;
    /**int* alloc = (int*) sys_mmap();
    int* alloc2 = (int*) sys_mmap();
    alloc[0] = 1337;
    alloc2[0] = 7890;
    sys_munmap((void*)alloc);
    sys_munmap((void*)alloc2);*/

    /**alloc2 = (int*) sys_mmap();*/
    while( 1 ) {
        v1++;
    }
}

void user_process2()
{

    int v2 = -12;
    while( 1 ){
        v2 -= 2;
    }
}

void user_process3()
{
	// Provoque un data abort
	//int * pt = (int *) 0x1100001;
	//*pt = 12;
	
    int* alloc = (int*) sys_mmap(2);
    alloc[0] = 1337;
    int * alloc2 = (int*) sys_mmap(1);
    int * alloc3 = (int*) sys_mmap(1);
    int * alloc4 = (int*) sys_mmap(2);
    alloc4[0]=339;
    alloc3[0]=5;
    alloc2[0] = 12345;


    sys_munmap((void*)alloc2, 1);
    sys_munmap((void*)alloc4, 2);

    int v3 = 0;
    while( 1 ) {
        v3 += 5;
    }
}


void
kmain(void){

    /** Exemple de sortie console **/
    uart_init();
    uart_send_str("hello\n");

    sched_init();

    //create_process((func_t*) &user_process1);
    //create_process((func_t*) &user_process2);
    create_process((func_t*) &user_process3);

    timer_init();
    // Activation des interruptions
    ENABLE_IRQ();

    __asm("cps 0x10"); // switch CPU to USER mode




    //uint32_t  tr = vmem_translate(0x8000, 0);



    while(1) {
        sys_yield();
    }
}
