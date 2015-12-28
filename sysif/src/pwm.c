#include "pwm.h"
#include "hw.h"
#include "sched.h"
#include "kheap.h"

extern char _binary_tune_wav_start;
extern char _binary_tune_wav_end;


static volatile unsigned* gpio = (void*)GPIO_BASE;
static volatile unsigned* clk = (void*)CLOCK_BASE;
static volatile unsigned* pwm = (void*)PWM_BASE;

static int compteur_incrementation = 0;
static unsigned int increment_div_1000 = 1570; // contrôle la vitesse de lecture increment/1000 => astuce pour éviter les divisions 
static unsigned int indice_volume = 0; // contrôle le volume (de 0 à 4) 

char* audio_data = &_binary_tune_wav_start;
static char* audio_data_volumes[NOMBRE_DE_NIVEAUX_VOLUME];
static unsigned long long longueur_piste_audio;

/*********************************************************************************************************
Méthodes statiques
*********************************************************************************************************/
static void
pause(int t) {
    // Pause for about t ms
    int i;
    for (;t>0;t--) {
        for (i=5000;i>0;i--){
            i++; i--;
        }
    }
}

static int
divise(int x, int y) {
    int quotient = 0;
    int coef = 1;
    if(x < 0)
    {
        x = -x;
        coef *= -1;
    }
    if(y < 0)
    {
        y = -y;
        coef *= -1;
    }
    while (x >= y) {
        x -= y;
        quotient++;
    }
    return quotient * coef;
}

static uint8_t
get_min_uint8(char* data)
{
    uint8_t res = 255;
    unsigned long long i = 0;
    for (; i < longueur_piste_audio; ++i)
    {
        if(data[i] < res)
        {
            res = data[i];
        }
    }
    return res;
}

static uint8_t
get_max_uint8(char* data)
{
    uint8_t res = 0;
    unsigned long long i = 0;
    for (; i < longueur_piste_audio; ++i)
    {
        if(data[i] > res)
        {
            res = data[i];
        }
    }
    return res;
}

static void
cree_niveaux_volumes(void)
{
    // note : 0x80=128 = volume 0
    int i = 0;
    for(; i<NOMBRE_DE_NIVEAUX_VOLUME ; i++)
    {
        audio_data_volumes[i] = (char*) kAlloc(sizeof(char) * longueur_piste_audio);
    }
    
    uint8_t max = get_max_uint8(audio_data); 
    uint8_t min = get_min_uint8(audio_data);
    unsigned long long indice_dans_la_musique = 0;
    for(; indice_dans_la_musique < longueur_piste_audio ; indice_dans_la_musique++)
    {
        uint8_t actu = audio_data[indice_dans_la_musique];
        uint8_t incr = (uint8_t)divise(110, NOMBRE_DE_NIVEAUX_VOLUME);
        for(i=0 ; i<NOMBRE_DE_NIVEAUX_VOLUME ; ++i)
        {        
            audio_data_volumes[i][indice_dans_la_musique] = actu + (uint8_t)divise( ((255 - incr*i - max)*(actu - min) - (min - incr*i)*(max - actu)), (max - min) );
        }
    }

}

static void
audio_init(void)
{
    /* Values read from raspbian: */
    /* PWMCLK_CNTL = 148 = 10010100
       PWMCLK_DIV = 16384
       PWM_CONTROL=9509 = 10010100100101
       PWM0_RANGE=1024
       PWM1_RANGE=1024 */

    longueur_piste_audio = &_binary_tune_wav_end - &_binary_tune_wav_start;

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

    cree_niveaux_volumes();
}

static int
get_incr(void)
{
    int res = 0;
    compteur_incrementation += increment_div_1000;
    while(compteur_incrementation > 1000)
    {
        compteur_incrementation -= 1000;
        res++;
    }
    return res;
}

/*********************************************************************************************************
Méthodes publiques
*********************************************************************************************************/

void
lance_audio(void)
{
    unsigned long long i=0;
    long status;
    audio_init();

    for(;;)
    {
        // led_blink();
        i=0;
        while (i < longueur_piste_audio)
        {
            
            status =  *(pwm + BCM2835_PWM_STATUS);
            if (!(status & BCM2835_FULL1))
            {
                *(pwm+BCM2835_PWM_FIFO) = (char)(audio_data_volumes[indice_volume][(unsigned long long)i]);
                i += get_incr();
            }
            else
            {
                /* on passe la main en attendant que (status & BCM2835_FULL1)=0 => pas plein
                 (!=0 => plein) */
                sys_wait(PROCESS_DETAILS_WAITING_PWM_FIFO);
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
configuration_audio(void)
{
    for(;;)
    {
        // simulation attente utilisateur
        sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
        // simulation gestion vitesse de lecture
        /* if(increment_div_1000 == 570)
        {
            increment_div_1000 = 1570;
        }
        else
        {
            increment_div_1000 = 570;
        }*/
        // simulation changement de volume
        indice_volume = (indice_volume + 1) % NOMBRE_DE_NIVEAUX_VOLUME;
    }
}

