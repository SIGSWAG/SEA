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
b *do_sys_yield
b *user_process
b *sys_yield

source utils.gdb

continue
