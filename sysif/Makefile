#############################################################################
# 4IF SEA "SysIF" Makefile
#############################################################################

default: kernel_for_qemu

all: kernel_for_qemu kernel_for_sdcard

kernel_for_qemu: build/kernel.elf build/kernel.list

kernel_for_sdcard: kernel_for_qemu build/kernel.img 

remake: clean all

# options à passer au compilateur C
CFLAGS=-Wall -Werror -nostdlib -nostartfiles -ffreestanding -std=c99 -g -fomit-frame-pointer -nostartfiles -O0 -fdiagnostics-show-option

# options à passer à la fois au compilateur C et à l'assembleur
COMMON_FLAGS=-mcpu=arm1176jzf-s

# Object files (excluding kmain)
OBJECTS=$(addsuffix .o,  $(addprefix build/, $(basename $(notdir $(wildcard src/*.[cs]))))) mario_music_ghosts.o mario_jump.o mario_gameover.o mario_1UP.o mario_mushroom.o mario_win.o

# non-default usage: "make KMAIN=test/my-kmain.c" 
KMAIN ?= ./kmain.c

# check whether kmain does exists. typically this would get triggered
# by a command like "make KMAIN=" (i.e. with no value for KMAIN)
ifneq "$(shell test -f ''${KMAIN} && echo yes || echo no)" "yes"
    $(error no such file: "${KMAIN}")
endif

# check whether we're dealing with a different kmain.c than last time:
# either updated/modified, or maybe another file, located somewhere else
ifneq "$(shell md5sum ${KMAIN})" "$(shell test -f build/kmain.md5 && cat build/kmain.md5)"
  # if so, then our kmain.o has to be recompiled
  .PHONY: build/kmain.o
endif

build:
	mkdir -p build

# compilation C vers ELF
build/%.o: src/%.c $(wildcard src/*.h) | build
	@echo "COMPILATION DE $<"
	arm-none-eabi-gcc $(COMMON_FLAGS) $(CFLAGS) $< -c -o $@

# compilation du point d'entrée 
build/kmain.o: $(KMAIN) $(wildcard src/*.h)
	arm-none-eabi-gcc $(COMMON_FLAGS) $(CFLAGS) -I src $< -c -o $@
	@#"let's silently remember the checksum of the most recent kmain.c we compiled"
	@md5sum $(KMAIN) > build/kmain.md5

# assemblage .s vers ELF
build/%.o: src/%.s | build
	arm-none-eabi-as -g $(COMMON_FLAGS) $< -c -o $@

# édition de liens
build/kernel.elf: $(OBJECTS) build/kmain.o
	arm-none-eabi-ld $^ -o $@ -T src/sysif.ld -Map build/mapfile.map

# conversion de l'image pour transfert sur carte SD
build/kernel.img: build/kernel.elf
	arm-none-eabi-objcopy $^ -O binary $@

# désassemblage
build/kernel.list: build/kernel.elf
	arm-none-eabi-objdump -d -j .text -j .bss -j .stack -j .kernel_heap -j .user_stacks -j .data $< > $@

mario_music_ghosts.o: mario_music_ghosts.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^

mario_jump.o: mario_jump.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^

mario_gameover.o: mario_gameover.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^

mario_1UP.o: mario_1UP.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^

mario_mushroom.o: mario_mushroom.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^

mario_win.o: mario_win.wav
	arm-none-eabi-ld -s -r -o $@ -b binary $^


# python dump_music_separated.py -o $@ -f "$^"
# mario.o: mario.wav
#	arm-none-eabi-ld -s -r -o $@ -b binary $^


# nettoyage: effacer tous les fichiers générés
.PHONY:clean
clean:
	rm -rf build
	rm -f mario_music_ghosts.o
	rm -f mario_jump.o
	rm -f mario_gameover.o
	rm -f mario_1UP.o
	rm -f mario_mushroom.o
	rm -f mario_win.o
