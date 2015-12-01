#include "msp430.h"
#include "lcd.h"

#define SEG_A 0x01   
#define SEG_B 0x02  //      -A-        
#define SEG_C 0x04  //     F   B       
#define SEG_D 0x08  //      -G-        
#define SEG_E 0x40  //     E   C       
#define SEG_F 0x10  //      -D-        
#define SEG_G 0x20
#define SEG_H 0x80

static const char lcd_digits[] = { // shape of digits
  SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F, // "0"
  SEG_B + SEG_C,                // "1"
  SEG_A + SEG_B + SEG_D + SEG_E + SEG_G, // "2"
  SEG_A + SEG_B + SEG_C + SEG_D + SEG_G, // "3"
  SEG_B + SEG_C + SEG_F + SEG_G, // "4"
  SEG_A + SEG_C + SEG_D + SEG_F + SEG_G, // "5"
  SEG_A + SEG_C + SEG_D + SEG_E + SEG_F + SEG_G, // "6"
  SEG_A + SEG_B + SEG_C, // "7"
  SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F + SEG_G, // "8"
  SEG_A + SEG_B + SEG_C + SEG_D + SEG_F + SEG_G, // "9"
  };

// LCD Constants
#define LCD_NUM_DIGITS  7                   // Number of digits on screen
#define LCD_MEM_OFFSET  1                   // First digit is not LCDMEM[0]

void lcd_display_digit(int pos, int digit)
{
    if(pos>LCD_NUM_DIGITS)
        return;
    
    // the first two bytes of  lcd controller memory are not mapped to
    // any item on the lcd screen istelf
    LCDMEM[LCD_MEM_OFFSET+pos]=lcd_digits[digit];
}


void lcd_display_number(unsigned int number)
{
    lcd_clear();
    if(number == 0 )
        lcd_display_digit(1, 0);
    
    int i=1;
    while(number)
    {
        lcd_display_digit(i,number%10);
        number /= 10;
        i++;
    }
}


/*  Initialize the LCD_A controller
    
    claims P5.2-P5.4, P8, P9, and P10.0-P10.5
    assumes ACLK to be default 32khz (LFXT1)
*/
void lcd_init()
{
  // our LCD screen is a SBLCDA4 => 4-mux operation (SLAU213 p4)
  
  // 4-mux operation needs all  4 common lines (COM0-COM3). COM0 has
  // a dedicated pin  (pin 52, cf SLAS508H p3),  but let's claim the
  // other three.
  P5DIR |= (BIT4 | BIT3 | BIT2); // pins are output direction
  P5SEL |= (BIT4 | BIT3 | BIT2); // select 'peripheral' function (VS GPIO)
  
  // Configure LCD controller
  LCDACTL = 0b01011101;
  // bit 0 turns on the LCD_A module
  // bit 1 unused
  // bit 2 enables LCD segments
  // bits 3-4 set LCD mux rate: 4-mux
  // bits 5-7 set frequency
  
  // Configure port pins
  //
  // mappings  are  detailed  on  SLAU213  p15: our  screen  has  22
  // segments, wired to MCU pins S4 to S25 (shared with GPIO P8, P9,
  // and P10.0 to P10.5)
  LCDAPCTL0 = 0b01111110;
  // bit 0: MCU S0-S3   => not connected to the screen
  // bit 1: MCU S4-S7   => screen pins S0-S3   (P$14-P$11)
  // bit 2: MCU S8-S11  => screen pins S4-S7   (P$10-P$7)
  // bit 3: MCU S12-S15 => screen pins S8-S11  (P$6 -P$3)
  // bit 4: MCU S16-S19 => screen pins S12-S15 (P$2, P$1, P$19, P$20)
  // bit 5: MCU S20-S23 => screen pins S16-S19 (P$21-P$24)
  // bit 6: MCU S24-S25 => screen pins S20-21  (P$25, P$26)
  // bit 7: MCU S28-S31 => not connected to the screen
  
  LCDAPCTL1 = 0 ; // MCU S32-S39 => not connected to the screen
  
  // Note: as we do not intend to support battery-powered operation,
  // we don't need to mess with charge pumps and such.
  
  // clear all LCD  memory (cf SLAU056J p. 26-4)
  int j;
  for( j=0 ; j<20 ; j++)
    {
      LCDMEM[j] = 0x00;
    }
}

void lcd_clear()
{
  int j;

  // clear all the video memory
  for( j=0 ; j<20 ; j++)
    {
      LCDMEM[j] = 0;
    }
}
