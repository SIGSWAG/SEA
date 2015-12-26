#include "pwm.h"
#include "hw.h"
#include "sched.h"

extern char _binary_tune_wav_start;
extern char _binary_tune_wav_end;


static volatile unsigned* gpio = (void*)GPIO_BASE;
static volatile unsigned* clk = (void*)CLOCK_BASE;
static volatile unsigned* pwm = (void*)PWM_BASE;

static unsigned long long increment = 1;
static int volume = 1;
/* Decomment this in order to get sound */
char* audio_data = &_binary_tune_wav_start;

static void pause(int t) {
    // Pause for about t ms
    int i;
    for (;t>0;t--) {
        for (i=5000;i>0;i--){
            i++; i--;
        }
    }
}


static void audio_init(void)
{
    /* Values read from raspbian: */
    /* PWMCLK_CNTL = 148 = 10010100
       PWMCLK_DIV = 16384
       PWM_CONTROL=9509 = 10010100100101
       PWM0_RANGE=1024
       PWM1_RANGE=1024 */

    unsigned int range = 0x400;
    unsigned int idiv = 2;
    /* unsigned int pwmFrequency = (19200000 / idiv) / range; */

    SET_GPIO_ALT(40, 0);
    SET_GPIO_ALT(45, 0);
    pause(2);

    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5);    // stop clock
    *(clk + BCM2835_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12);  // set divisor
    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;      // enable + oscillator (raspbian has this as plla)

    pause(2); 

    // disable PWM
    *(pwm + BCM2835_PWM_CONTROL) = 0;
       
    pause(2);

    *(pwm+BCM2835_PWM0_RANGE) = range;
    *(pwm+BCM2835_PWM1_RANGE) = range;

    *(pwm+BCM2835_PWM_CONTROL) =
    BCM2835_PWM1_USEFIFO | // Use FIFO and not PWM mode
    //          BCM2835_PWM1_REPEATFF |
    BCM2835_PWM1_ENABLE  | // enable channel 1
    BCM2835_PWM0_USEFIFO | // use FIFO and not PWM mode
    //          BCM2835_PWM0_REPEATFF |  */
    1<<6                 | // clear FIFO
    BCM2835_PWM0_ENABLE;   // enable channel 0

    pause(2);
}

void
audio_test()
{
    unsigned long long i=0;
    long status;
    audio_init();
    
    unsigned long long size = &_binary_tune_wav_end - &_binary_tune_wav_start;

    for(;;)
    {
        i=0;
        while (i < size)
        {
            
            status =  *(pwm + BCM2835_PWM_STATUS);
            if (!(status & BCM2835_FULL1))
            {
                *(pwm+BCM2835_PWM_FIFO) = (char)(audio_data[(unsigned long long)i] * volume);
                i+=increment;
            }
            else
            {
                /* wait sur (status & BCM2835_FULL1) : =0 => pas plein ; !=0 => plein */
                sys_yield();
            }
              
            if ((status & ERRORMASK))
            {
                //                uart_print("error: ");
                //                hexstring(status);
                //                uart_print("\r\n");
                *(pwm+BCM2835_PWM_STATUS) = ERRORMASK;
            }
        }
    }
}



void
audio_config()
{
    while(1)
    {
        pause(1000); // simulation d'attente entre 2 config
        // increment = (increment) % 2 + 1;
        // volume = (volume)%2 + 1;
        sys_yield(); // wait à implémenter
    }
}

