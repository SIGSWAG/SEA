#include "pwm.h"
#include "hw.h"
#include "sched.h"
#include "kheap.h"

#define NB_MUSIQUES 6

extern char _binary_mario_music_ghosts_wav_end;
extern char _binary_mario_music_ghosts_wav_start;
extern char _binary_mario_jump_wav_end;
extern char _binary_mario_jump_wav_start;
extern char _binary_mario_gameover_wav_end;
extern char _binary_mario_gameover_wav_start;
extern char _binary_mario_1UP_wav_end;
extern char _binary_mario_1UP_wav_start;
extern char _binary_mario_mushroom_wav_end;
extern char _binary_mario_mushroom_wav_start;
extern char _binary_mario_win_wav_end;
extern char _binary_mario_win_wav_start;

static unsigned int musique_courante = 0;
static struct musique_infos playlist[NB_MUSIQUES];

static volatile unsigned* gpio = (void*)GPIO_BASE;
static volatile unsigned* clk = (void*)CLOCK_BASE;
static volatile unsigned* pwm = (void*)PWM_BASE;

static unsigned int indice_volume = NOMBRE_DE_NIVEAUX_VOLUME_MIN; 

static unsigned int musique_arretee = 0;
static unsigned int musique_prete = 0;

/*********************************************************************************************************
Méthodes statiques
*********************************************************************************************************/
static void
pause_physique(int t) {
    // Pause for about t ms
    int i;
    for (;t>0;t--) {
        for (i=5000;i>0;i--){
            i++; i--;
        }
    }
}
/*
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
*/


// static void unsigned_divide(unsigned int dividend,
//              unsigned int divisor,
//              unsigned int* quotient,
//              unsigned int* remainder )
// {
//   unsigned int t, num_bits;
//   unsigned int q, bit, d;
//   int i;

//   *remainder = 0;
//   *quotient = 0;

//   if (divisor == 0)
//     return;

//   if (divisor > dividend) {
//     *remainder = dividend;
//     return;
//   }

//   if (divisor == dividend) {
//     *quotient = 1;
//     return;
//   }

//   num_bits = 32;

//   while (*remainder < divisor) {
//     bit = (dividend & 0x80000000) >> 31;
//     *remainder = (*remainder << 1) | bit;
//     d = dividend;
//     dividend = dividend << 1;
//     num_bits--;
//   }


//   /* The loop, above, always goes one iteration too far.
//      To avoid inserting an "if" statement inside the loop
//      the last iteration is simply reversed. */

//   dividend = d;
//   *remainder = *remainder >> 1;
//   num_bits++;

//   for (i = 0; i < num_bits; i++) {
//     bit = (dividend & 0x80000000) >> 31;
//     *remainder = (*remainder << 1) | bit;
//     t = *remainder - divisor;
//     q = !((t & 0x80000000) >> 31);
//     dividend = dividend << 1;
//     *quotient = (*quotient << 1) | q;
//     if (q) {
//        *remainder = t;
//      }
//   }
// }  /* unsigned_divide */




#if ENABLE_VOLUME
    static unsigned divide(unsigned dividend, unsigned divisor) { 

        unsigned denom=divisor;
        unsigned current = 1;
        unsigned answer=0;

        if ( denom > dividend) 
            return 0;

        if ( denom == dividend)
            return 1;

        while (denom <= dividend) {
            denom <<= 1;
            current <<= 1;
        }

        denom >>= 1;
        current >>= 1;

        while (current!=0) {
            if ( dividend >= denom) {
                dividend -= denom;
                answer |= current;
            }
            current >>= 1;
            denom >>= 1;
        }    
        return answer;
    }

    #define ABS(x)  ((x) < 0 ? -(x) : (x))

    static int signed_divide(int dividend,
               int divisor)
    {
      unsigned int dend, dor;

      dend = ABS(dividend);
      dor  = ABS(divisor);
      

      /* the sign of the remainder is the same as the sign of the dividend
         and the quotient is negated if the signs of the operands are
         opposite */
      int quotient = divide( dend, dor);
      
      if (dividend < 0) {
        // int remainder = -r;
        if (divisor > 0)
          quotient = -quotient;
      }
      else { /* positive dividend */
        // int remainder = r;
        if (divisor < 0)
          quotient = -quotient;
      }
        return quotient;  
    } /* signed_divide */

    static uint8_t
    get_min_uint8(char* data, unsigned int numero_musique)
    {
        uint8_t res = 255;
        unsigned long long i = 0;
        for (; i < playlist[numero_musique].longueur_piste_audio; ++i)
        {
            if(data[i] < res)
            {
                res = data[i];
            }
        }
        return res;
    }

    static uint8_t
    get_max_uint8(char* data, unsigned int numero_musique)
    {
        uint8_t res = 0;
        unsigned long long i = 0;
        for (; i < playlist[numero_musique].longueur_piste_audio; ++i)
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
        int j = 0;
        for(; j<NB_MUSIQUES ; ++j)
        {
            for(i=0; i<NOMBRE_DE_NIVEAUX_VOLUME ; ++i)
            {
                playlist[j].audio_data_volumes[i] = (char*) kAlloc(sizeof(char) * playlist[j].longueur_piste_audio);
            }
        
            uint8_t max = get_max_uint8(playlist[j].music_wav_start, j); 
            uint8_t min = get_min_uint8(playlist[j].music_wav_start, j);
            unsigned long long indice_dans_la_musique = 0;
            for(; indice_dans_la_musique < playlist[j].longueur_piste_audio ; ++indice_dans_la_musique)
            {
                uint8_t actu = playlist[j].music_wav_start[indice_dans_la_musique];
                uint8_t incr = (uint8_t)signed_divide(110, NOMBRE_DE_NIVEAUX_VOLUME);
                for(i=0 ; i<NOMBRE_DE_NIVEAUX_VOLUME ; ++i)
                {
                    playlist[j].audio_data_volumes[i][indice_dans_la_musique] = (uint8_t)((int)actu + signed_divide( ((255 - incr*i - max)*(actu - min) - (min - incr*i)*(max - actu)), (int)(max - min) ));
                }
            }
        }
    }
#else
    static void
    cree_niveaux_volumes(void)
    {
        int i = 0;
        int j = 0;
        for(; j<NB_MUSIQUES ; ++j)
        {
            for(i=0; i<NOMBRE_DE_NIVEAUX_VOLUME ; ++i)
            {
                playlist[j].audio_data_volumes[i] = playlist[j].music_wav_start;
            }
        }    
    }
#endif

void
init_materiel(void)
{
    unsigned int range = 0x400;
    // unsigned int range = 0x488;
    unsigned int idiv = 2;
    unsigned int fdiv = 512;
    /* unsigned int pwmFrequency = (19 200 000 / idiv) / range; */

    SET_GPIO_ALT(40, 0);
    SET_GPIO_ALT(45, 0);
    pause_physique(2);

    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | (1 << 5);    // stop clock
    *(clk + BCM2835_PWMCLK_DIV)  = PM_PASSWORD | (idiv<<12) | fdiv;  // set divisor
    *(clk + BCM2835_PWMCLK_CNTL) = PM_PASSWORD | 16 | 1;      // enable + oscillator (raspbian has this as plla)


    pause_physique(2); 

    // disable PWM
    *(pwm + BCM2835_PWM_CONTROL) = 0;
    
    pause_physique(2);

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

    pause_physique(2);
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

    playlist[0].music_wav_end = &_binary_mario_music_ghosts_wav_end;
    playlist[0].music_wav_start = &_binary_mario_music_ghosts_wav_start;
    playlist[1].music_wav_end = &_binary_mario_jump_wav_end;
    playlist[1].music_wav_start = &_binary_mario_jump_wav_start;
    playlist[2].music_wav_end = &_binary_mario_gameover_wav_end;
    playlist[2].music_wav_start = &_binary_mario_gameover_wav_start;
    playlist[3].music_wav_end = &_binary_mario_1UP_wav_end;
    playlist[3].music_wav_start = &_binary_mario_1UP_wav_start;
    playlist[4].music_wav_end = &_binary_mario_mushroom_wav_end;
    playlist[4].music_wav_start = &_binary_mario_mushroom_wav_start;
    playlist[5].music_wav_end = &_binary_mario_win_wav_end;
    playlist[5].music_wav_start = &_binary_mario_win_wav_start;

    init_materiel();

    int i = 0;
    for(; i<NB_MUSIQUES ; ++i)
    {
        playlist[i].longueur_piste_audio = playlist[i].music_wav_end - playlist[i].music_wav_start;
        playlist[i].compteur_incrementation = 0;
        playlist[i].increment_div_1000 = 2300;
        playlist[i].position_lecture_musique = 0;
        playlist[i].musique_arretee = 0;
        playlist[i].musique_prete = 0;
    }

    cree_niveaux_volumes();
}

static int
get_incr(unsigned int musique_a_lire)
{
    int res = 0;
    playlist[musique_a_lire].compteur_incrementation += playlist[musique_a_lire].increment_div_1000;
    while(playlist[musique_a_lire].compteur_incrementation > 1000)
    {
        playlist[musique_a_lire].compteur_incrementation -= 1000;
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
    long status;
    audio_init();
    musique_prete = 1;
    unsigned int musique_a_lire = musique_courante;
#if LEAP_MOTION
    musique_stop();
    led_on();
    sys_wait(PROCESS_DETAILS_MUSIC_PAUSE);
    led_off();
#else
    led_on();
    sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
    led_off();
#endif

    for(;;)
    {
        playlist[musique_a_lire].position_lecture_musique = 0;
        while (playlist[musique_a_lire].position_lecture_musique < playlist[musique_a_lire].longueur_piste_audio)
        {
            // gestion de la pause et de l'arrêt
            if(musique_arretee)
            {
                sys_wait(PROCESS_DETAILS_MUSIC_PAUSE);
            }

            // gestion des erreurs (du status actuel de la fifo de son)
            status =  *(pwm + BCM2835_PWM_STATUS);
            if (!(status & BCM2835_FULL1))
            {
                *(pwm+BCM2835_PWM_FIFO) = (char)(playlist[musique_a_lire].audio_data_volumes[indice_volume][(unsigned long long)playlist[musique_a_lire].position_lecture_musique]);
                playlist[musique_a_lire].position_lecture_musique += get_incr(musique_a_lire);
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
            musique_a_lire = musique_courante;
        }
    }
}

void
musique_pause(void)
{
    musique_arretee = 1;
}

void
musique_lecture(void)
{
    musique_arretee = 0;
}

void
musique_stop(void)
{
    musique_arretee = 1;
    playlist[musique_courante].position_lecture_musique = 0;
}

unsigned int
musique_est_arretee(void)
{
    return musique_arretee;
}

unsigned int
musique_est_prete(void)
{
    return musique_prete;
}

static void
set_increment_musique(int increment)
{
    if(increment >= 300 && increment < 5000 ){
        playlist[musique_courante].increment_div_1000 = increment;
    }
}

void
augmenter_vitesse(void){
    if(playlist[musique_courante].increment_div_1000 >= 2300){
        set_increment_musique(playlist[musique_courante].increment_div_1000 + 100);
    }
    else{
        set_increment_musique(playlist[musique_courante].increment_div_1000 + 80);
    }
}

void
diminuer_vitesse(void){
    if(playlist[musique_courante].increment_div_1000 > 2300){
        set_increment_musique(playlist[musique_courante].increment_div_1000 - 100);
    }
    else{
        set_increment_musique(playlist[musique_courante].increment_div_1000 - 80);
    }
}

void
set_volume(int volume)
{
    if(volume >= 0 && volume < NOMBRE_DE_NIVEAUX_VOLUME)
    {
        indice_volume = volume;
    }
}

void
augmenter_volume(void)
{
    if(indice_volume > 0)
    {
        indice_volume--;
    }
}

void
diminuer_volume(void)
{
    if(indice_volume < NOMBRE_DE_NIVEAUX_VOLUME - 1 )
    {
        indice_volume++;
    }
}

void
musique_suivante(void)
{
    musique_courante = (musique_courante >= NB_MUSIQUES-1) ? 0 : musique_courante+1;
    playlist[musique_courante].position_lecture_musique = 0;
}

void
musique_precedente(void)
{
    musique_courante = (musique_courante <= 0) ? NB_MUSIQUES-1 : musique_courante-1;
    playlist[musique_courante].position_lecture_musique = 0;
}

void
configuration_audio(void)
{
    int cpt = 0;
    for(;;)
    {

        // simulation pause
        /*
        // simulation attente utilisateur
        sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
        if(musique_est_prete() == 1)
        {
            musique_pause();
            sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
            musique_lecture();            
        }
        */

        // simulation gestion vitesse de lecture
        /* 
        // simulation attente utilisateur
        sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
        if(increment_div_1000 == 570)
        {
            increment_div_1000 = 1570;
        }
        else
        {
            increment_div_1000 = 570;
        }
        */
        
        // simulation changement de volume
        indice_volume = (indice_volume + 1) % NOMBRE_DE_NIVEAUX_VOLUME;

        // simulation de stop
        cpt++;
        sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
        if(cpt > 2)
        {
            cpt = 0;
            if(musique_est_prete() == 1)
            {
                musique_stop();
                sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);
                musique_lecture();                
            }
        }
    }
}

