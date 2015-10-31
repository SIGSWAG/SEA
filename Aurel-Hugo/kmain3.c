void
dummy()
{
	return;
}

int
div(int dividend, int divisor)
{
	int result = 0;
	int remainder = dividend;
	
	while (remainder >= divisor) {
		result++;
		remainder -= divisor;
	}
	
	return result;
}

int
compute_volume(int rad)
{
	int rad3 = rad * rad * rad;
	
	return div(4*355*rad3, 3*113);
}

int
kmain( void )
{
	//__asm("mrs r3, spsr");
	// __asm("cps #0b10000");
	__asm("cps #0b10011");
	int radius = 5;
	__asm("mov r3, %0" : : "r"(radius));
	__asm("mov %0, r3" : "=r"(radius));
	int volume;
	
	dummy();
	volume = compute_volume(radius);
	
	return volume;
}
