// Centrale inertielle MPU6050 (module GY-521), lue registre par registre, sans bibliothèque.
// Ce fichier est identique aux étapes 2 et 4.
//
// Repère du drone : X vers l'avant, Y vers la gauche, Z vers le haut.
// Monte le GY-521 composants vers le haut, flèche X imprimée vers l'avant du drone
// (sinon règle IMU_ROTATION dans config.h).
//
// Conventions de signe, les mêmes partout dans le projet :
//   roulis  > 0 : le côté droit descend
//   tangage > 0 : le nez monte
//   lacet   > 0 : le nez tourne vers la droite (vu de dessus)
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>

// Angle (sens horaire vu de dessus) entre l'avant du drone et la flèche X du GY-521 : 0, 90, 180 ou 270
#ifndef IMU_ROTATION
#define IMU_ROTATION 0
#endif

// Part du gyroscope dans le filtre complémentaire (le reste vient de l'accéléromètre).
// Plus proche de 1 = moins sensible aux vibrations, mais dérive corrigée plus lentement.
#ifndef IMU_ALPHA
#define IMU_ALPHA 0.996f
#endif

class CentraleInertielle {
public:
  float accX = 0, accY = 0, accZ = 0;               // en g
  float vitRoulis = 0, vitTangage = 0, vitLacet = 0; // en °/s
  float roulis = 0, tangage = 0;                    // angles filtrés en degrés, sans trim
  float trimRoulis = 0, trimTangage = 0;            // angles mesurés quand le drone est à plat

  bool demarrer(int sda, int scl) {
    Wire.begin(sda, scl, 400000);
    uint8_t id = lireRegistre(0x75);  // WHO_AM_I
    if (id == 0x00 || id == 0xFF) return false;
    if (id != 0x68) Serial.printf("[IMU] WHO_AM_I = 0x%02X au lieu de 0x68 (clone ?), on continue\n", id);
    ecrireRegistre(0x6B, 0x80);  // remise à zéro
    delay(100);
    ecrireRegistre(0x6B, 0x01);  // réveil, horloge sur le gyroscope X
    ecrireRegistre(0x1A, 0x03);  // filtre passe-bas interne ~44 Hz
    ecrireRegistre(0x1B, 0x08);  // gyroscope ±500 °/s (65,5 pas par °/s)
    ecrireRegistre(0x1C, 0x10);  // accéléromètre ±8 g (4096 pas par g)
    delay(50);
    return true;
  }

  // Mesure le décalage du gyroscope. Le drone doit rester immobile.
  // Renvoie false si ça a bougé pendant la mesure : il suffit de recommencer.
  bool calibrerGyro(int echantillons = 1000) {
    float somme[3] = {0, 0, 0};
    int16_t mini[3] = {32767, 32767, 32767}, maxi[3] = {-32768, -32768, -32768};
    for (int i = 0; i < echantillons; i++) {
      int16_t a[3], g[3];
      if (!lireBrut(a, g)) return false;
      for (int k = 0; k < 3; k++) {
        somme[k] += g[k];
        mini[k] = min(mini[k], g[k]);
        maxi[k] = max(maxi[k], g[k]);
      }
      delay(1);
    }
    for (int k = 0; k < 3; k++) {
      if (maxi[k] - mini[k] > 300) return false;  // plus de ~4,5 °/s d'écart : le drone a bougé
    }
    for (int k = 0; k < 3; k++) decalageGyro[k] = somme[k] / echantillons;
    initialise = false;
    return true;
  }

  // À appeler à intervalle régulier ; dt en secondes.
  bool mettreAJour(float dt) {
    int16_t a[3], g[3];
    if (!lireBrut(a, g)) return false;

    float ax = a[0] / 4096.0f, ay = a[1] / 4096.0f, az = a[2] / 4096.0f;
    float gx = (g[0] - decalageGyro[0]) / 65.5f;
    float gy = (g[1] - decalageGyro[1]) / 65.5f;
    float gz = (g[2] - decalageGyro[2]) / 65.5f;
    tourner(ax, ay);
    tourner(gx, gy);

    accX = ax; accY = ay; accZ = az;
    vitRoulis = gx;     // rotation autour de X : le côté gauche monte = roulis positif
    vitTangage = -gy;   // rotation positive autour de Y : le nez descend
    vitLacet = -gz;     // rotation positive autour de Z : le nez part à gauche

    float accRoulis = atan2f(ay, az) * RAD_TO_DEG;
    float accTangage = atan2f(ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;

    if (!initialise) {
      roulis = accRoulis;
      tangage = accTangage;
      initialise = true;
      return true;
    }
    roulis += vitRoulis * dt;
    tangage += vitTangage * dt;
    // On ne fait confiance à l'accéléromètre que s'il mesure à peu près la gravité seule
    float norme = sqrtf(ax * ax + ay * ay + az * az);
    if (norme > 0.75f && norme < 1.25f) {
      roulis = IMU_ALPHA * roulis + (1.0f - IMU_ALPHA) * accRoulis;
      tangage = IMU_ALPHA * tangage + (1.0f - IMU_ALPHA) * accTangage;
    }
    return true;
  }

  float roulisCorrige() const { return roulis - trimRoulis; }
  float tangageCorrige() const { return tangage - trimTangage; }

  // Le trim est gardé en mémoire flash : il survit aux redémarrages et aux nouveaux téléversements.
  void chargerTrim() {
    Preferences prefs;
    if (prefs.begin("drone", true)) {
      trimRoulis = prefs.getFloat("trimR", 0);
      trimTangage = prefs.getFloat("trimT", 0);
      prefs.end();
    }
  }

  void enregistrerTrim() {
    trimRoulis = roulis;
    trimTangage = tangage;
    Preferences prefs;
    prefs.begin("drone", false);
    prefs.putFloat("trimR", trimRoulis);
    prefs.putFloat("trimT", trimTangage);
    prefs.end();
  }

private:
  static const uint8_t ADRESSE = 0x68;  // 0x69 si la broche AD0 du GY-521 est à 3,3 V
  float decalageGyro[3] = {0, 0, 0};
  bool initialise = false;

  static void tourner(float &x, float &y) {
#if IMU_ROTATION == 90
    float t = x; x = y; y = -t;
#elif IMU_ROTATION == 180
    x = -x; y = -y;
#elif IMU_ROTATION == 270
    float t = x; x = -y; y = t;
#endif
  }

  void ecrireRegistre(uint8_t registre, uint8_t valeur) {
    Wire.beginTransmission(ADRESSE);
    Wire.write(registre);
    Wire.write(valeur);
    Wire.endTransmission();
  }

  uint8_t lireRegistre(uint8_t registre) {
    Wire.beginTransmission(ADRESSE);
    Wire.write(registre);
    if (Wire.endTransmission(false) != 0) return 0xFF;
    if (Wire.requestFrom(ADRESSE, (uint8_t)1) != 1) return 0xFF;
    return Wire.read();
  }

  // Lit d'un coup accéléromètre (3 axes), température (ignorée) et gyroscope (3 axes)
  bool lireBrut(int16_t a[3], int16_t g[3]) {
    Wire.beginTransmission(ADRESSE);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(ADRESSE, (uint8_t)14) != 14) return false;
    uint8_t o[14];
    for (int i = 0; i < 14; i++) o[i] = Wire.read();
    for (int k = 0; k < 3; k++) {
      a[k] = (int16_t)((o[2 * k] << 8) | o[2 * k + 1]);
      g[k] = (int16_t)((o[8 + 2 * k] << 8) | o[9 + 2 * k]);
    }
    return true;
  }
};
