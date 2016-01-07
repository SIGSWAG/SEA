#include "syscall.h"
#include "sched.h"
#include "vmem.h"
#include "util.h"
#include "hw.h"
#include "asm_tools.h"

static uint32_t * sp_param_base;
static uint32_t lr_irq;



void __attribute__((naked)) irq_handler() {

    __asm("mov %0, lr" : "=r"(lr_irq));

    // Switch to SVC
    __asm("cps 0x13");

    // On push lr_irq - 4 dans la pile
    __asm("push {%0}": : "r"(lr_irq - 4));
    // Sauvegarde du contexte
    __asm("stmfd sp!, {r0-r12}");

    //recuperation de l'adresse de SP
    __asm("mov %0, sp" : "=r"(sp_param_base));

    // On configure la MMU avec la table des pages système
    invalidate_TLB();
    configure_mmu_kernel();

    // Changement de contexte
    do_sys_yield(sp_param_base);

    // On reconfigure la MMU avec la table des pages du nouveau processus élu

    // Restitution du contexte
    __asm("ldmfd sp!, {r0-r12}");
    // On récupère lr_irq (on l'a pushé avant)
    __asm("pop {%0}" : "=r"(lr_irq));

    // On réarme le timer + active les interruptions
    set_next_tick_default();
    ENABLE_TIMER_IRQ();
    ENABLE_IRQ();



    invalidate_TLB();
    configure_mmu_C((unsigned int)current_process->page_table);

    // Switch to user
    __asm("cps 0x10");
    // On jump au lr_irq (décrémenté de 4 avant)
    __asm("mov pc, %0": :"r"(lr_irq));
}

void __attribute__((naked)) swi_handler() {


    // Sauvegarde du contexte
    __asm("stmfd sp!, {r0-r12, lr}");
    //recuperation de l'adresse de SP
    __asm("mov %0, sp" : "=r"(sp_param_base));
    int code = 0;
    __asm("mov %0, r0" : "=r"(code));
    //On configure la MMU avec la table des pages système
    invalidate_TLB();
    configure_mmu_kernel();


    switch(code){
    case 1:
        do_sys_reboot();
        break;
    case 2:
        do_sys_nop();
        break;
    case 3:
        do_sys_settime();
        break;
    case 4:
        do_sys_gettime();
        break;
    case 5:
        do_sys_yieldto(sp_param_base);
        break;
    case 6:
        do_sys_yield(sp_param_base);
        break;
    case 7:
        do_sys_exit(sp_param_base);
        break;
    case 8:
        do_sys_mmap(sp_param_base);
        break;
    case 9:
        do_sys_munmap(sp_param_base);
        break;
    case 10 :
        do_sys_gmalloc(sp_param_base);
        break;
    case 11 :
        do_sys_gfree(sp_param_base);
        break;
    case 12 :
        do_sys_wait(sp_param_base);
        break;
    default :
        PANIC();
    }


    invalidate_TLB();
    configure_mmu_C((unsigned int)current_process->page_table);

    // Restitution du contexte et pop du lr sauvegardé avant dans le pc + changement de mode (retour vers le mode précédent)
    // Le chapeau permet de restituer CPSR en utilisant le SPSR
    __asm("ldmfd sp!, {r0-r12, pc}^");

}

void sys_reboot() {
    __asm("mov r0, #1");
    __asm("SWI #0");
}

void do_sys_reboot() {
    /** for emulator **/
    __asm("mov pc, #0x8000");

    /** for Raspberry **/
    /*
        const int PM_RSTC = 0x2010001c;
        const int PM_WDOG = 0x20100024;
        const int PM_PASSWORD = 0x5a000000;
        const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;

        PUT32(PM_WDOG, PM_PASSWORD | 1);
        PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
        */
    while(1);
}

void sys_nop() {
    __asm("mov r0, #2");
    //int lr_stock = 0;
    //__asm("mov %0, lr" : "=r"(lr_stock));

    __asm("SWI #0");

    // restitution qui marche au cazou
    //__asm("cps #31");

    //__asm("mov lr, %0" : : "r"(lr_stock));
}

void do_sys_nop() {

}

void sys_settime(uint64_t date_ms) {
    __asm("mov r0, #3");
    // On enregistre date_ms dans un registre avant le SWI
    uint32_t date_lowbits = date_ms & (0x00000000FFFFFFFF);
    uint32_t date_highbits = date_ms >> 32;
    __asm("mov r1, %0" : : "r"(date_lowbits));
    __asm("mov r2, %0" : : "r"(date_highbits));
    __asm("SWI #0");
}

void do_sys_settime() {
    uint64_t date_ms = ((uint64_t)*(sp_param_base + 2) << 32) | (*(sp_param_base + 1) & (0x00000000FFFFFFFF));

    set_date_ms(date_ms);
}

uint64_t sys_gettime() {
    __asm("mov r0, #4");
    __asm("SWI #0");

    uint64_t date_ms = ((uint64_t)*(sp_param_base + 1) << 32) | (*(sp_param_base + 0) & (0x00000000FFFFFFFF));

    return date_ms;
}

void do_sys_gettime() {
    uint64_t date_ms = get_date_ms();
    uint32_t date_lowbits = date_ms & (0x00000000FFFFFFFF);
    uint32_t date_highbits = date_ms >> 32;
    /*
        __asm("mov %0, %1" : "=r"(*sp_param_base) : "r"(date_lowbits));
        __asm("mov %0, %1" : "=r"(*(sp_param_base+1)) : "r"(date_highbits));
        */
    sp_param_base[0] = date_lowbits;
    sp_param_base[1] = date_highbits;

}


void* sys_mmap(int nbPages)
{
    __asm("mov r0, #8");
    __asm("mov r1, %0" : : "r"(nbPages));
    __asm("SWI #0");


    return (void*) sp_param_base[0];
}

void* gmalloc(int nbBytes)
{
    __asm("mov r0, #10");
    __asm("mov r1, %0" : : "r"(nbBytes));
    __asm("SWI #0");


    return (void*) sp_param_base[0];
}



void do_sys_gmalloc(uint32_t * sp_param){

    void* ret = do_gmalloc(current_process, (int)sp_param[1]);
    sp_param[0] = (uint32_t) ret;

}



void do_sys_mmap(uint32_t * sp_param){

    void* ret = vmem_alloc_for_userland(current_process, (int)sp_param[1]);
    sp_param[0] = (uint32_t) ret;

}

void sys_munmap(void* pointer, int nbPages){

    __asm("mov r0, #9");
    __asm("mov r1, %0" : : "r"(pointer));
    __asm("mov r2, %0" : : "r"(nbPages));
    __asm("SWI #0");


}

void gfree(void* pointer){

    __asm("mov r0, #11");
    __asm("mov r1, %0" : : "r"(pointer));
    __asm("SWI #0");


}

void do_sys_gfree(uint32_t * sp_param){


    void * pointer = (void*) sp_param[1];
    do_gfree(current_process, pointer);


}

void do_sys_munmap(uint32_t * sp_param){


    void * pointer = (void*) sp_param[1];
    int nbPages = (int)sp_param[2];
    vmem_desalloc_for_userland(current_process, pointer, nbPages);


}



