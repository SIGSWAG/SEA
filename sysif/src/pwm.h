#ifndef PWM_H
#define PWM_H

#define NOMBRE_DE_NIVEAUX_VOLUME 1

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

#endif
