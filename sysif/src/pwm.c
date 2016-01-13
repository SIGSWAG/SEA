#include "pwm.h"
#include "hw.h"
#include "sched.h"
#include "kheap.h"

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
    }
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
                char intensite_initiale = (char)(playlist[musique_a_lire].music_wav_start[(unsigned long long)playlist[musique_a_lire].position_lecture_musique]);
                *(pwm+BCM2835_PWM_FIFO) = temporisation_volumes[(uint8_t)intensite_initiale][indice_volume];
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
    // int cpt = 0;
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
        sys_wait(PROCESS_DETAILS_WAITING_1_SECOND);

        /*
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
        */
    }
}

