CFLAGS=-O1 -Wall -mmcu=msp430fg4618 -mcpu=430
DEPS = lcd.h
OBJS = lcd.o chrono.o 


%.o: %.c $(DEPS)
	msp430-gcc $(CFLAGS) -c -o $@ $< 

chrono.elf: $(OBJS)
	msp430-gcc $(CFLAGS) -o $@ $^ 

chrono.lst: chrono.elf
	msp430-objdump -D $^ > $@

.PHONY: clean

clean:
	rm -f *.o *.elf *.lst *~
