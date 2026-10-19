// Pilotage des 4 moteurs brushed (coreless 8520) par MOSFET, en PWM.
// Ce fichier est identique aux étapes 3 et 4.
//
// Disposition en X, vue de dessus, nez vers le haut :
//
//      M1 (AVG)  M2 (AVD)
//       horaire   anti-horaire
//            \   /
//             [ ]
//            /   \
//      M4 (ARG)  M3 (ARD)
//   anti-horaire  horaire
#pragma once
#include <Arduino.h>
#include <esp_arduino_version.h>

const int NB_MOTEURS = 4;
const int BROCHES_MOTEURS[NB_MOTEURS] = {5, 6, 7, 10};  // M1 AVG, M2 AVD, M3 ARD, M4 ARG
const char *const NOMS_MOTEURS[NB_MOTEURS] = {"M1 avant gauche", "M2 avant droit", "M3 arriere droit", "M4 arriere gauche"};

const uint32_t FREQUENCE_PWM = 20000;  // 20 kHz : au-dessus de l'audible
const uint8_t RESOLUTION_PWM = 10;     // 0 à 1023

inline void moteursDemarrer() {
  for (int i = 0; i < NB_MOTEURS; i++) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(BROCHES_MOTEURS[i], FREQUENCE_PWM, RESOLUTION_PWM);
    ledcWrite(BROCHES_MOTEURS[i], 0);
#else
    ledcSetup(i, FREQUENCE_PWM, RESOLUTION_PWM);
    ledcAttachPin(BROCHES_MOTEURS[i], i);
    ledcWrite(i, 0);
#endif
  }
}

// valeur de 0 (arrêt) à 1000 (plein gaz)
inline void moteurEcrire(int moteur, int valeur) {
  uint32_t rapport = (uint32_t)constrain(valeur, 0, 1000) * 1023 / 1000;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(BROCHES_MOTEURS[moteur], rapport);
#else
  ledcWrite(moteur, rapport);
#endif
}

inline void moteursArreter() {
  for (int i = 0; i < NB_MOTEURS; i++) moteurEcrire(i, 0);
}

// Mélange gaz + corrections vers les 4 moteurs.
// Une correction positive fait varier l'angle dans le sens positif (voir mpu6050.h) :
//   roulis  > 0 : moteurs gauches plus vite, le côté droit descend
//   tangage > 0 : moteurs avant plus vite, le nez monte
//   lacet   > 0 : moteurs anti-horaires plus vite, le cadre tourne en horaire (nez à droite)
inline void moteursMelanger(float gaz, float roulis, float tangage, float lacet, int sortie[NB_MOTEURS]) {
  sortie[0] = lroundf(gaz + roulis + tangage - lacet);  // M1 AVG, horaire
  sortie[1] = lroundf(gaz - roulis + tangage + lacet);  // M2 AVD, anti-horaire
  sortie[2] = lroundf(gaz - roulis - tangage - lacet);  // M3 ARD, horaire
  sortie[3] = lroundf(gaz + roulis - tangage + lacet);  // M4 ARG, anti-horaire
}
