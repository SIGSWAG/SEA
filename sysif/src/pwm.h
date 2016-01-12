#ifndef PWM_H
#define PWM_H

#define ENABLE_VOLUME 0
#define NB_MUSIQUES 6
#define NOMBRE_DE_NIVEAUX_VOLUME 10
#define NOMBRE_DE_NIVEAUX_VOLUME_MIN 4
#define LEAP_MOTION 1
#define MUSIQUE_MODE_PLAYLIST 0
#define MUSIQUE_MODE_PARALLEL 1
#define BASE_INCREMENT 2300

void lance_audio(void);
void configuration_audio(void);

void musique_pause(void);
void musique_stop(void);
void musique_lecture(void);
void set_volume(int volume);
void augmenter_vitesse(void);
void diminuer_vitesse(void);
void augmenter_volume(void);
void diminuer_volume(void);
unsigned int musique_est_arretee(void);
unsigned int musique_est_prete(void);
void musique_suivante(void);
void musique_precedente(void);
void init_materiel(void);
unsigned int get_mode_musique(void);
void play_ponctuel(unsigned int mode);

struct musique_infos{
	int compteur_incrementation;
	unsigned int increment_div_1000;
	unsigned long long position_lecture_musique;
	char* audio_data_volumes[NOMBRE_DE_NIVEAUX_VOLUME];
	unsigned long long longueur_piste_audio;
	char* music_wav_start;
	char* music_wav_end;
};


#endif
