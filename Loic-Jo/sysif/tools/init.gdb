# -*- mode:gdb-script -*-

layout split
focus cmd
winheight cmd 25
target remote:1234

set history filename ~/.gdb_history
set history save

b *reset_asm_handler
b *after_kmain
b kernel_panic
b *kernel_panic
b swi_handler

b *kmain
b *user_process
b sched.c:47
b *sys_exit
b *do_sys_exit
b kmain.c:15

source utils.gdb

continue
