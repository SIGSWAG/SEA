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

void sys_audio_init();

void do_sys_audio_init();


long sys_get_audio_status();

void do_sys_get_audio_status();

void sys_set_pwm_delta(char value, int delta);

void do_sys_set_pwm_delta();

#endif
