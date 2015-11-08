#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

void sys_reboot(void);
void sys_nop(void);
void sys_settime(uint64_t date_ms);
uint64_t sys_gettime(void);

void do_sys_reboot(void);
void do_sys_nop(void);
void do_sys_settime(void);
void do_sys_gettime(void);

void swi_handler(void);
void irq_handler(void);

uint64_t get64(unsigned int* stack, unsigned int low, unsigned int high);
void set64(unsigned int* stack, unsigned int registre, uint64_t value);

#endif
