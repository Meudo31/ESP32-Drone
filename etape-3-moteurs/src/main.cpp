// Drone ESP32 - étape 3 : banc d'essai des moteurs. À FAIRE SANS HÉLICES.
// Le drone reçoit la télécommande de l'étape 1 et fait tourner les 4 moteurs :
// mêmes gaz pour tous, plus une petite part des manches pour vérifier le mélange.
// Pas encore de stabilisation : on vérifie le câblage, le sens de rotation et la batterie.
//
// Commandes à taper dans le moniteur série (drone désarmé uniquement) :
//   1 à 4 : fait tourner un seul moteur doucement pendant 1,5 s
//   0     : arrête tout
#include <Arduino.h>
#include "config.h"
#include "protocole.h"
#include "moteurs.h"

const int BROCHE_LED = 8;       // allumée à l'état bas
const int BROCHE_BATTERIE = 1;  // pont diviseur 100 kΩ / 100 kΩ
const uint32_t DELAI_PERTE_RADIO_MS = 300;
const uint16_t GAZ_ARMEMENT_MAX = 50;  // on n'arme que si le manche des gaz est en bas

portMUX_TYPE verrou = portMUX_INITIALIZER_UNLOCKED;
PaquetCommande commandeRecue = {};
uint32_t derniereCommandeMs = 0;
bool uneCommandeRecue = false;

uint8_t etat = ETAT_DESARME;
bool interrupteurPrecedent = true;  // au démarrage, il faut d'abord voir l'interrupteur sur arrêt
float batterieMv = 0;

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

void mesurerBatterie() {
  float mv = analogReadMilliVolts(BROCHE_BATTERIE) * RAPPORT_PONT_BATTERIE;
  batterieMv = batterieMv == 0 ? mv : batterieMv * 0.9f + mv * 0.1f;  // lissage
}

// Armement : il faut un front montant de l'interrupteur, gaz en bas.
// Toute coupure (perte radio) oblige à repasser l'interrupteur sur arrêt avant de réarmer.
void gererArmement(const PaquetCommande &c, bool lienOk) {
  bool interrupteur = c.arme;
  if (!lienOk) {
    if (etat == ETAT_ARME) Serial.println("!!! Perte radio : moteurs coupes");
    if (uneCommandeRecue) etat = ETAT_PERTE_RADIO;
    interrupteurPrecedent = true;  // il faudra repasser par "désarmé"
    return;
  }
  if (!interrupteur) {
    if (etat == ETAT_ARME) Serial.println("Desarme");
    etat = ETAT_DESARME;
  } else if (!interrupteurPrecedent) {
    if (c.gaz <= GAZ_ARMEMENT_MAX) {
      etat = ETAT_ARME;
      Serial.println("ARME : attention, les moteurs vont tourner");
    } else {
      etat = ETAT_REFUS;
      Serial.println("Armement refuse : baisse les gaz puis repasse l'interrupteur sur arret");
    }
  }
  interrupteurPrecedent = interrupteur;
}

void testerMoteur(int moteur) {
  Serial.printf("Test %s : il doit tourner %s\n", NOMS_MOTEURS[moteur],
                (moteur == 0 || moteur == 2) ? "dans le sens HORAIRE (vu de dessus)" : "dans le sens ANTI-HORAIRE (vu de dessus)");
  moteurEcrire(moteur, 150);
  delay(1500);
  moteursArreter();
}

void lireCommandesSerie() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '0') moteursArreter();
    if (c < '1' || c > '4') continue;
    if (etat == ETAT_ARME) {
      Serial.println("Desarme d'abord le drone pour tester un moteur seul");
      continue;
    }
    testerMoteur(c - '1');
  }
}

void setup() {
  moteursDemarrer();  // en premier : les moteurs restent à l'arrêt quoi qu'il arrive
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);  // sans câble USB, ne jamais bloquer sur l'affichage
  pinMode(BROCHE_LED, OUTPUT);
  digitalWrite(BROCHE_LED, HIGH);
  analogSetPinAttenuation(BROCHE_BATTERIE, ADC_11db);
  delay(1500);
  Serial.println();
  Serial.println("=== Etape 3 : banc moteurs (SANS HELICES !) ===");
  if (!radioDemarrer(surReception)) {
    Serial.println("ERREUR : ESP-NOW n'a pas demarre");
    while (true) delay(1000);
  }
  Serial.println("Tape 1, 2, 3 ou 4 pour tester un moteur seul.");
}

void loop() {
  static uint32_t dernierTourMs = 0, derniereTelemetrieMs = 0, dernierAffichageMs = 0;
  static uint16_t numeroTelemetrie = 0;
  uint32_t maintenant = millis();
  if (maintenant - dernierTourMs < 5) return;  // 200 Hz suffit pour le banc
  dernierTourMs = maintenant;

  portENTER_CRITICAL(&verrou);
  PaquetCommande c = commandeRecue;
  bool lienOk = uneCommandeRecue && (maintenant - derniereCommandeMs) < DELAI_PERTE_RADIO_MS;
  portEXIT_CRITICAL(&verrou);

  gererArmement(c, lienOk);
  mesurerBatterie();

  int sortie[NB_MOTEURS] = {0, 0, 0, 0};
  if (etat == ETAT_ARME) {
    float gaz = c.gaz * GAZ_MAX_BANC / 1000.0f;
    // Sur le banc, manche roulis à droite = on veut pencher à droite, etc.
    // Le tangage est inversé : manche poussé = nez qui descend = tangage négatif.
    moteursMelanger(gaz, c.roulis * GAIN_MANCHES_BANC, -c.tangage * GAIN_MANCHES_BANC,
                    c.lacet * GAIN_MANCHES_BANC, sortie);
    for (int i = 0; i < NB_MOTEURS; i++) {
      sortie[i] = c.gaz > GAZ_ARMEMENT_MAX ? constrain(sortie[i], 0, GAZ_MAX_BANC) : 0;
      moteurEcrire(i, sortie[i]);
    }
  } else {
    moteursArreter();
    lireCommandesSerie();
  }

  // LED : éteinte = désarmé, allumée = armé, clignote = perte radio ou refus
  bool clignote = (maintenant / 100) % 2;
  bool led = etat == ETAT_ARME || ((etat == ETAT_PERTE_RADIO || etat == ETAT_REFUS) && clignote);
  digitalWrite(BROCHE_LED, led ? LOW : HIGH);

  if (maintenant - derniereTelemetrieMs >= 100) {
    derniereTelemetrieMs = maintenant;
    PaquetTelemetrie t = {};
    t.cle = CLE_APPAIRAGE;
    t.type = TYPE_TELEMETRIE;
    t.etat = etat;
    t.batterieMv = (uint16_t)batterieMv;
    if (batterieMv < BATTERIE_FAIBLE_MV) t.alertes |= ALERTE_BATTERIE_FAIBLE;
    t.numero = numeroTelemetrie++;
    radioEnvoyer(&t, sizeof(t));
  }

  if (Serial && maintenant - dernierAffichageMs >= 500) {
    dernierAffichageMs = maintenant;
    Serial.printf("%-15s gaz %4u | M1 %4d  M2 %4d  M3 %4d  M4 %4d | batterie %.2f V%s\n",
                  nomEtat(etat), c.gaz, sortie[0], sortie[1], sortie[2], sortie[3],
                  batterieMv / 1000.0f, batterieMv < BATTERIE_FAIBLE_MV ? " FAIBLE" : "");
  }
}
