// Drone ESP32 - étape 4 : contrôleur de vol complet, mode stabilisé (angle)
// Boucle à 250 Hz : centrale inertielle -> PID d'angle -> PID de vitesse -> mélange -> moteurs.
// La télécommande reste celle de l'étape 1.
//
// Sécurités : armement seulement gaz en bas et drone à plat, coupure immédiate des moteurs
// en cas de perte radio, de chute (angle trop grand) ou d'erreur du MPU6050.
//
// Commandes à taper dans le moniteur série (drone désarmé uniquement) :
//   n : enregistre la position actuelle comme "à plat" (trim)
//   c : recalibre le gyroscope
#include <Arduino.h>
#include "config.h"
#include "protocole.h"
#include "mpu6050.h"
#include "moteurs.h"

const int BROCHE_SDA = 4;
const int BROCHE_SCL = 3;
const int BROCHE_LED = 8;       // allumée à l'état bas
const int BROCHE_BATTERIE = 1;  // pont diviseur 100 kΩ / 100 kΩ
const uint32_t PERIODE_BOUCLE_US = 4000;   // 250 Hz
const uint32_t DELAI_PERTE_RADIO_MS = 300;
const uint16_t GAZ_ARMEMENT_MAX = 50;

// PID sur la mesure : la dérivée porte sur la mesure (pas l'erreur) pour éviter
// les à-coups quand le manche bouge, et elle est filtrée car le gyroscope vibre.
struct Pid {
  float kp, ki, kd, limiteI;
  float integrale = 0, mesurePrecedente = 0, deriveeFiltree = 0;
  bool premier = true;

  Pid(float p, float i, float d, float limite) : kp(p), ki(i), kd(d), limiteI(limite) {}

  float calculer(float consigne, float mesure, float dt) {
    float erreur = consigne - mesure;
    integrale = constrain(integrale + ki * erreur * dt, -limiteI, limiteI);
    float derivee = premier ? 0 : -(mesure - mesurePrecedente) / dt;
    premier = false;
    mesurePrecedente = mesure;
    deriveeFiltree += 0.25f * (derivee - deriveeFiltree);
    return kp * erreur + integrale + kd * deriveeFiltree;
  }

  void remettreAZero() {
    integrale = 0;
    deriveeFiltree = 0;
    premier = true;
  }
};

Pid pidRoulis(KP_ROULIS, KI_ROULIS, KD_ROULIS, LIMITE_INTEGRALE);
Pid pidTangage(KP_TANGAGE, KI_TANGAGE, KD_TANGAGE, LIMITE_INTEGRALE);
Pid pidLacet(KP_LACET, KI_LACET, KD_LACET, LIMITE_INTEGRALE);

CentraleInertielle imu;

portMUX_TYPE verrou = portMUX_INITIALIZER_UNLOCKED;
PaquetCommande commandeRecue = {};
uint32_t derniereCommandeMs = 0;
bool uneCommandeRecue = false;

uint8_t etat = ETAT_CALIBRATION;
bool interrupteurPrecedent = true;  // au démarrage, il faut d'abord voir l'interrupteur sur arrêt
float batterieMv = 0;
int sortie[NB_MOTEURS] = {0, 0, 0, 0};
uint32_t erreursImuDeSuite = 0, dureeMaxTourUs = 0;

void surReception(const uint8_t *donnees, int longueur) {
  if (longueur != sizeof(PaquetCommande)) return;
  PaquetCommande p;
  memcpy(&p, donnees, sizeof(p));
  if (p.cle != CLE_APPAIRAGE || p.type != TYPE_COMMANDE) return;
  portENTER_CRITICAL(&verrou);
  commandeRecue = p;
  derniereCommandeMs = millis();
  uneCommandeRecue = true;
  portEXIT_CRITICAL(&verrou);
}

void couper(uint8_t nouvelEtat, const char *raison) {
  moteursArreter();
  if (etat == ETAT_ARME && raison) Serial.printf("!!! Coupure moteurs : %s\n", raison);
  etat = nouvelEtat;
  interrupteurPrecedent = true;  // il faudra repasser l'interrupteur sur arrêt
}

void calibrer() {
  etat = ETAT_CALIBRATION;
  moteursArreter();
  Serial.println("Calibration du gyroscope : pose le drone a plat et ne le touche plus...");
  while (!imu.calibrerGyro()) {
    digitalWrite(BROCHE_LED, !digitalRead(BROCHE_LED));
  }
  for (int i = 0; i < 250; i++) {  // laisse le filtre se stabiliser
    imu.mettreAJour(PERIODE_BOUCLE_US * 1e-6f);
    delayMicroseconds(PERIODE_BOUCLE_US);
  }
  Serial.println("Gyroscope calibre.");
  etat = ETAT_DESARME;
  interrupteurPrecedent = true;
}

void gererArmement(const PaquetCommande &c, bool lienOk) {
  if (etat == ETAT_CALIBRATION) return;
  if (!lienOk) {
    if (etat == ETAT_ARME) couper(ETAT_PERTE_RADIO, "perte radio");
    else if (uneCommandeRecue && etat != ETAT_CHUTE) etat = ETAT_PERTE_RADIO;
    interrupteurPrecedent = true;
    return;
  }
  bool interrupteur = c.arme;
  if (!interrupteur) {
    if (etat == ETAT_ARME) Serial.println("Desarme");
    moteursArreter();
    etat = ETAT_DESARME;
  } else if (!interrupteurPrecedent) {
    bool aPlat = fabsf(imu.roulisCorrige()) < ANGLE_ARMEMENT_MAX && fabsf(imu.tangageCorrige()) < ANGLE_ARMEMENT_MAX;
    if (c.gaz <= GAZ_ARMEMENT_MAX && aPlat) {
      pidRoulis.remettreAZero();
      pidTangage.remettreAZero();
      pidLacet.remettreAZero();
      etat = ETAT_ARME;
      Serial.println("ARME");
    } else {
      etat = ETAT_REFUS;
      Serial.println(aPlat ? "Armement refuse : baisse les gaz puis repasse l'interrupteur sur arret"
                           : "Armement refuse : le drone n'est pas a plat");
    }
  }
  interrupteurPrecedent = interrupteur;
}

void piloter(const PaquetCommande &c, float dt) {
  float roulis = imu.roulisCorrige(), tangage = imu.tangageCorrige();
  if (fabsf(roulis) > ANGLE_CHUTE || fabsf(tangage) > ANGLE_CHUTE) {
    couper(ETAT_CHUTE, "chute detectee");
    return;
  }

  if (c.gaz < GAZ_DECOLLAGE) {
    // Au sol : pas de correction, sinon l'intégrale gonfle et le drone bascule au décollage
    pidRoulis.remettreAZero();
    pidTangage.remettreAZero();
    pidLacet.remettreAZero();
    for (int i = 0; i < NB_MOTEURS; i++) {
      sortie[i] = c.gaz > GAZ_ARMEMENT_MAX ? max((int)c.gaz, GAZ_RALENTI) : GAZ_RALENTI;
      moteurEcrire(i, sortie[i]);
    }
    return;
  }

  // Consignes : manche roulis à droite = pencher à droite ; manche poussé = piquer du nez
  float consigneRoulis = c.roulis * ANGLE_MAX / 500.0f;
  float consigneTangage = -c.tangage * ANGLE_MAX / 500.0f;
  float consigneLacet = c.lacet * VITESSE_LACET_MAX / 500.0f;

  // Boucle externe : erreur d'angle -> vitesse de rotation voulue
  float vitesseRoulisVoulue = KP_ANGLE * (consigneRoulis - roulis);
  float vitesseTangageVoulue = KP_ANGLE * (consigneTangage - tangage);

  // Boucle interne : vitesse voulue -> correction moteurs
  float corrRoulis = pidRoulis.calculer(vitesseRoulisVoulue, imu.vitRoulis, dt);
  float corrTangage = pidTangage.calculer(vitesseTangageVoulue, imu.vitTangage, dt);
  float corrLacet = pidLacet.calculer(consigneLacet, imu.vitLacet, dt);

  float gaz = map(c.gaz, 0, 1000, 0, GAZ_MAX);
  moteursMelanger(gaz, corrRoulis, corrTangage, corrLacet, sortie);
  for (int i = 0; i < NB_MOTEURS; i++) {
    sortie[i] = constrain(sortie[i], GAZ_RALENTI, 1000);
    moteurEcrire(i, sortie[i]);
  }
}

void lireCommandesSerie() {
  while (Serial.available()) {
    char c = Serial.read();
    if (etat == ETAT_ARME) continue;  // jamais de calibration en vol
    if (c == 'n') {
      imu.enregistrerTrim();
      Serial.printf("Trim enregistre : roulis %+.2f, tangage %+.2f\n", imu.trimRoulis, imu.trimTangage);
    } else if (c == 'c') {
      calibrer();
    }
  }
}

void setup() {
  moteursDemarrer();  // en premier : les moteurs restent à l'arrêt quoi qu'il arrive
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // sans câble USB, ne jamais bloquer la boucle de vol sur l'affichage
  pinMode(BROCHE_LED, OUTPUT);
  digitalWrite(BROCHE_LED, HIGH);
  analogSetPinAttenuation(BROCHE_BATTERIE, ADC_11db);
  delay(1500);
  Serial.println();
  Serial.println("=== Etape 4 : controleur de vol ===");

  while (!imu.demarrer(BROCHE_SDA, BROCHE_SCL)) {
    Serial.println("ERREUR : MPU6050 introuvable");
    digitalWrite(BROCHE_LED, !digitalRead(BROCHE_LED));
    delay(200);
  }
  imu.chargerTrim();
  calibrer();

  if (!radioDemarrer(surReception)) {
    Serial.println("ERREUR : ESP-NOW n'a pas demarre");
    while (true) delay(1000);
  }
  Serial.println("Pret. Gaz en bas, puis interrupteur d'armement sur marche.");
}

void loop() {
  static uint32_t dernierTourUs = micros();
  static uint32_t derniereTelemetrieMs = 0, dernierAffichageMs = 0, derniereBatterieMs = 0;
  static uint16_t numeroTelemetrie = 0;

  uint32_t debutTour = micros();
  if (debutTour - dernierTourUs < PERIODE_BOUCLE_US) return;
  float dt = (debutTour - dernierTourUs) * 1e-6f;
  dernierTourUs = debutTour;

  if (imu.mettreAJour(dt)) {
    erreursImuDeSuite = 0;
  } else if (++erreursImuDeSuite > 5) {
    couper(ETAT_CHUTE, "le MPU6050 ne repond plus");
  }

  uint32_t maintenant = millis();
  portENTER_CRITICAL(&verrou);
  PaquetCommande c = commandeRecue;
  bool lienOk = uneCommandeRecue && (maintenant - derniereCommandeMs) < DELAI_PERTE_RADIO_MS;
  portEXIT_CRITICAL(&verrou);

  gererArmement(c, lienOk);
  if (etat == ETAT_ARME) {
    piloter(c, dt);
  } else {
    moteursArreter();
    for (int i = 0; i < NB_MOTEURS; i++) sortie[i] = 0;
    lireCommandesSerie();
  }

  if (maintenant - derniereBatterieMs >= 20) {
    derniereBatterieMs = maintenant;
    float mv = analogReadMilliVolts(BROCHE_BATTERIE) * RAPPORT_PONT_BATTERIE;
    batterieMv = batterieMv == 0 ? mv : batterieMv * 0.95f + mv * 0.05f;
  }
  bool batterieFaible = batterieMv < BATTERIE_FAIBLE_MV;

  // LED : éteinte = désarmé, allumée = armé, clignotement rapide = coupure ou refus,
  // clignotement lent en vol = batterie faible, il est temps d'atterrir
  bool rapide = (maintenant / 100) % 2, lent = (maintenant / 400) % 2;
  bool led;
  if (etat == ETAT_ARME) led = batterieFaible ? lent : true;
  else led = (etat == ETAT_PERTE_RADIO || etat == ETAT_CHUTE || etat == ETAT_REFUS) && rapide;
  digitalWrite(BROCHE_LED, led ? LOW : HIGH);

  if (maintenant - derniereTelemetrieMs >= 100) {
    derniereTelemetrieMs = maintenant;
    PaquetTelemetrie t = {};
    t.cle = CLE_APPAIRAGE;
    t.type = TYPE_TELEMETRIE;
    t.etat = etat;
    t.alertes = batterieFaible ? ALERTE_BATTERIE_FAIBLE : 0;
    t.batterieMv = (uint16_t)batterieMv;
    t.roulis10 = (int16_t)(imu.roulisCorrige() * 10);
    t.tangage10 = (int16_t)(imu.tangageCorrige() * 10);
    t.numero = numeroTelemetrie++;
    radioEnvoyer(&t, sizeof(t));
  }

  if (Serial && maintenant - dernierAffichageMs >= 250) {
    dernierAffichageMs = maintenant;
    Serial.printf("%-15s roulis %+6.1f tangage %+6.1f lacet %+5.0f/s | gaz %4u | M %4d %4d %4d %4d | bat %.2f V | tour max %lu us\n",
                  nomEtat(etat), imu.roulisCorrige(), imu.tangageCorrige(), imu.vitLacet, c.gaz,
                  sortie[0], sortie[1], sortie[2], sortie[3], batterieMv / 1000.0f, (unsigned long)dureeMaxTourUs);
    dureeMaxTourUs = 0;
  }

  dureeMaxTourUs = max(dureeMaxTourUs, micros() - debutTour);
}
