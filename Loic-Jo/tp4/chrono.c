#include "msp430.h"
#include "legacymsp430.h"
#include "lcd.h"

unsigned int cpt;

// This will get executed 100 times per second
interrupt(TIMERA0_VECTOR) traitement_interruption_timer(void)
{
  lcd_display_number(cpt);
  cpt++;
}  

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;    // stop watchdog timer

    lcd_init(); // Initialize driver

    P5DIR |= 0x02 ; // Set P5.1 to output direction

    // initialization Timer_A
    TACTL = (2 << 8)  // TASSEL: clock source select. 10=SMCLK 
      |(1 << 4); // MC: mode control. 
    //  01=Up mode: the timer counts up to TACCR0 and repeats
    TACCR0 = 10485; // 100Hz for clock source SMCLK
    TACCTL0 = (1 << 4) ; // CCIE: Capture/compare interrupt enable. 1=enabled

    eint(); // enable interrupts (set GIE bit)
    
    cpt = 0;
    for(;;)
    {
       volatile unsigned int i;
       for(i=0;i<0xFFF0;i++) // software delay
       { // do nothing
       } 

       // lcd_display_number(cpt); //
       // cpt++; */

       P5OUT ^= 0x02 ; // toggle P5.1 (LED4)
    }
}
