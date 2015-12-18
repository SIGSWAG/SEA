#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

void sys_reboot();

void sys_nop();

void swi_handler();

void do_sys_reboot();

void do_sys_nop();

void sys_settime(uint64_t date_ms);

void do_sys_settime();

uint64_t sys_gettime();

void do_sys_gettime();

void irq_handler();

void* sys_mmap(int nbPages);

void do_sys_mmap(uint32_t * sp_param);

void sys_munmap(void* pointer, int nbPages);

void do_sys_munmap(uint32_t * sp_param);
#endif
