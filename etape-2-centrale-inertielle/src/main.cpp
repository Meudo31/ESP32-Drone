// Drone ESP32 - étape 2 : la centrale inertielle
// Lit le MPU6050, calcule roulis et tangage avec un filtre complémentaire et les envoie
// sur le port série au format Teleplot (extension VS Code), ou en texte.
//
// Commandes à taper dans le moniteur série :
//   n : enregistre la position actuelle comme "à plat" (trim, gardé en mémoire)
//   c : recalibre le gyroscope (drone immobile)
//   t : bascule entre l'affichage Teleplot et l'affichage texte
#include <Arduino.h>
#include "config.h"
#include "mpu6050.h"

const int BROCHE_SDA = 4;
const int BROCHE_SCL = 3;
const int BROCHE_LED = 8;  // allumée à l'état bas
const uint32_t PERIODE_BOUCLE_US = 4000;  // 250 Hz, comme le contrôleur de vol

CentraleInertielle imu;
bool modeTeleplot = true;
uint32_t erreursI2C = 0;

void calibrer() {
  Serial.println("Calibration du gyroscope : ne bouge plus le drone...");
  int essais = 0;
  while (!imu.calibrerGyro()) {
    digitalWrite(BROCHE_LED, !digitalRead(BROCHE_LED));
    if (++essais % 5 == 0) Serial.println("Ca bouge encore (ou le MPU6050 ne repond plus), on recommence...");
  }
  digitalWrite(BROCHE_LED, HIGH);
  Serial.println("Gyroscope calibre.");
}

void setup() {
  Serial.begin(115200);
  pinMode(BROCHE_LED, OUTPUT);
  digitalWrite(BROCHE_LED, HIGH);
  delay(1500);  // le temps que le port USB apparaisse
  Serial.println();
  Serial.println("=== Etape 2 : centrale inertielle ===");
  while (!imu.demarrer(BROCHE_SDA, BROCHE_SCL)) {
    Serial.println("ERREUR : MPU6050 introuvable. Verifie SDA -> GPIO4, SCL -> GPIO3, VCC -> 3V3, GND.");
    delay(1000);
  }
  imu.chargerTrim();
  Serial.printf("Trim en memoire : roulis %+.2f, tangage %+.2f\n", imu.trimRoulis, imu.trimTangage);
  calibrer();
}

void lireCommandes() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') {
      imu.enregistrerTrim();
      Serial.printf("Trim enregistre : roulis %+.2f, tangage %+.2f\n", imu.trimRoulis, imu.trimTangage);
    } else if (c == 'c') {
      calibrer();
    } else if (c == 't') {
      modeTeleplot = !modeTeleplot;
    }
  }
}

void afficher() {
  static uint32_t dernierAffichageMs = 0;
  uint32_t periode = modeTeleplot ? 20 : 500;
  if (millis() - dernierAffichageMs < periode) return;
  dernierAffichageMs = millis();
  if (modeTeleplot) {
    Serial.printf(">roulis:%.1f\n>tangage:%.1f\n>vitRoulis:%.0f\n>vitTangage:%.0f\n>vitLacet:%.0f\n",
                  imu.roulisCorrige(), imu.tangageCorrige(), imu.vitRoulis, imu.vitTangage, imu.vitLacet);
  } else {
    Serial.printf("Roulis %+6.1f  Tangage %+6.1f  | vitesses %+5.0f %+5.0f %+5.0f deg/s  | acc %+.2f %+.2f %+.2f g  | erreurs I2C %lu\n",
                  imu.roulisCorrige(), imu.tangageCorrige(), imu.vitRoulis, imu.vitTangage, imu.vitLacet,
                  imu.accX, imu.accY, imu.accZ, (unsigned long)erreursI2C);
  }
}

void loop() {
  static uint32_t dernierTourUs = micros();
  uint32_t maintenant = micros();
  if (maintenant - dernierTourUs < PERIODE_BOUCLE_US) return;
  float dt = (maintenant - dernierTourUs) * 1e-6f;
  dernierTourUs = maintenant;

  if (!imu.mettreAJour(dt)) erreursI2C++;
  lireCommandes();
  afficher();
}
