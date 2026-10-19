// Drone ESP32 - télécommande (ESP32 DevKit + 2 joysticks + interrupteur d'armement)
// Envoie les manches au drone 50 fois par seconde en ESP-NOW et affiche la télémétrie reçue.
#include <Arduino.h>
#include "config.h"
#include "protocole.h"

// Entrées analogiques : uniquement l'ADC1 (GPIO32 à 39), l'ADC2 ne marche pas avec la radio
const int BROCHE_GAZ = 34;      // joystick gauche, axe vertical
const int BROCHE_LACET = 35;    // joystick gauche, axe horizontal
const int BROCHE_TANGAGE = 32;  // joystick droit, axe vertical
const int BROCHE_ROULIS = 33;   // joystick droit, axe horizontal
const int BROCHE_ARMEMENT = 25; // interrupteur vers GND, fermé = armé
const int BROCHE_LED = 2;       // LED bleue de la carte : allumée = le drone répond

const uint32_t PERIODE_ENVOI_MS = 20;      // 50 paquets par seconde
const uint32_t DELAI_LIEN_PERDU_MS = 500;

int centreLacet, centreTangage, centreRoulis;
// L'interrupteur doit passer par "désarmé" après le démarrage avant de pouvoir armer :
// une télécommande allumée interrupteur sur marche n'arme jamais le drone.
bool armementAutorise = false;
uint16_t numeroPaquet = 0;

portMUX_TYPE verrou = portMUX_INITIALIZER_UNLOCKED;
PaquetTelemetrie telemetrie;
uint32_t derniereTelemetrieMs = 0;
bool telemetrieRecue = false;

void surReception(const uint8_t *donnees, int longueur) {
  if (longueur != sizeof(PaquetTelemetrie)) return;
  PaquetTelemetrie p;
  memcpy(&p, donnees, sizeof(p));
  if (p.cle != CLE_APPAIRAGE || p.type != TYPE_TELEMETRIE) return;
  portENTER_CRITICAL(&verrou);
  telemetrie = p;
  derniereTelemetrieMs = millis();
  telemetrieRecue = true;
  portEXIT_CRITICAL(&verrou);
}

int lireMoyenne(int broche, int nombre) {
  long somme = 0;
  for (int i = 0; i < nombre; i++) somme += analogRead(broche);
  return somme / nombre;
}

// Axe à ressort : centre -> 0, butées -> ±500
int16_t axeCentre(int brut, int centre, bool inverser) {
  int ecart = brut - centre;
  if (abs(ecart) < ZONE_MORTE) return 0;
  float v;
  if (ecart > 0) v = (float)(ecart - ZONE_MORTE) / (4095 - centre - ZONE_MORTE);
  else v = (float)(ecart + ZONE_MORTE) / (centre - ZONE_MORTE);
  int r = constrain((int)(v * 500), -500, 500);
  return inverser ? -r : r;
}

uint16_t axeGaz(int brut) {
  int r = constrain(map(brut, GAZ_ADC_MIN, GAZ_ADC_MAX, 0, 1000), 0, 1000);
  return INVERSER_GAZ ? 1000 - r : r;
}

void setup() {
  Serial.begin(115200);
  pinMode(BROCHE_ARMEMENT, INPUT_PULLUP);
  pinMode(BROCHE_LED, OUTPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  delay(300);

  Serial.println();
  Serial.println("=== Telecommande drone ===");
  Serial.println("Ne touche pas aux manches : mesure des centres...");
  centreLacet = lireMoyenne(BROCHE_LACET, 64);
  centreTangage = lireMoyenne(BROCHE_TANGAGE, 64);
  centreRoulis = lireMoyenne(BROCHE_ROULIS, 64);
  Serial.printf("Centres : lacet %d, tangage %d, roulis %d\n", centreLacet, centreTangage, centreRoulis);

  if (!radioDemarrer(surReception)) {
    Serial.println("ERREUR : ESP-NOW n'a pas demarre");
    while (true) { digitalWrite(BROCHE_LED, !digitalRead(BROCHE_LED)); delay(100); }
  }
  Serial.printf("Radio prete sur le canal %d, adresse MAC %s\n", CANAL_WIFI, WiFi.macAddress().c_str());
}

void loop() {
  static uint32_t dernierEnvoiMs = 0, dernierAffichageMs = 0;
  uint32_t maintenant = millis();
  if (maintenant - dernierEnvoiMs < PERIODE_ENVOI_MS) return;
  dernierEnvoiMs = maintenant;

  int brutGaz = lireMoyenne(BROCHE_GAZ, 4);
  bool interrupteur = digitalRead(BROCHE_ARMEMENT) == LOW;
  if (!interrupteur) armementAutorise = true;

  PaquetCommande c = {};
  c.cle = CLE_APPAIRAGE;
  c.type = TYPE_COMMANDE;
  c.arme = (interrupteur && armementAutorise) ? 1 : 0;
  c.gaz = axeGaz(brutGaz);
  c.roulis = axeCentre(lireMoyenne(BROCHE_ROULIS, 4), centreRoulis, INVERSER_ROULIS);
  c.tangage = axeCentre(lireMoyenne(BROCHE_TANGAGE, 4), centreTangage, INVERSER_TANGAGE);
  c.lacet = axeCentre(lireMoyenne(BROCHE_LACET, 4), centreLacet, INVERSER_LACET);
  c.numero = numeroPaquet++;
  radioEnvoyer(&c, sizeof(c));

  portENTER_CRITICAL(&verrou);
  PaquetTelemetrie t = telemetrie;
  bool recue = telemetrieRecue;
  uint32_t age = maintenant - derniereTelemetrieMs;
  portEXIT_CRITICAL(&verrou);
  bool lienOk = recue && age < DELAI_LIEN_PERDU_MS;
  digitalWrite(BROCHE_LED, lienOk ? HIGH : LOW);

  if (maintenant - dernierAffichageMs >= 500) {
    dernierAffichageMs = maintenant;
    Serial.printf("Gaz %4u (brut %4d)  Roulis %+4d  Tangage %+4d  Lacet %+4d  Arme %s",
                  c.gaz, brutGaz, c.roulis, c.tangage, c.lacet,
                  c.arme ? "OUI" : (interrupteur ? "bloque (repasse sur OFF)" : "non"));
    if (lienOk) {
      Serial.printf("  | Drone : %s, batterie %.2f V, roulis %+.1f, tangage %+.1f%s\n",
                    nomEtat(t.etat), t.batterieMv / 1000.0f, t.roulis10 / 10.0f, t.tangage10 / 10.0f,
                    (t.alertes & ALERTE_BATTERIE_FAIBLE) ? "  BATTERIE FAIBLE !" : "");
    } else {
      Serial.println("  | Drone : pas de reponse");
    }
  }
}
