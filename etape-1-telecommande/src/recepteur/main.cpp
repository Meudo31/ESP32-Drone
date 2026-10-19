// Drone ESP32 - étape 1 : récepteur d'essai (ESP32-C3 SuperMini, rien de branché)
// Affiche les commandes reçues, compte les paquets perdus et renvoie une télémétrie
// pour que la LED de la télécommande s'allume. Sert aussi de test de portée.
#include <Arduino.h>
#include "config.h"
#include "protocole.h"

const int BROCHE_LED = 8;  // LED de la carte, allumée à l'état bas
const uint32_t DELAI_PERTE_RADIO_MS = 300;

portMUX_TYPE verrou = portMUX_INITIALIZER_UNLOCKED;
PaquetCommande commande = {};
uint32_t derniereCommandeMs = 0;
bool commandeRecue = false;
uint32_t paquetsRecus = 0, paquetsPerdus = 0;
uint16_t numeroAttendu = 0;

void surReception(const uint8_t *donnees, int longueur) {
  if (longueur != sizeof(PaquetCommande)) return;
  PaquetCommande p;
  memcpy(&p, donnees, sizeof(p));
  if (p.cle != CLE_APPAIRAGE || p.type != TYPE_COMMANDE) return;
  portENTER_CRITICAL(&verrou);
  if (commandeRecue) {
    uint16_t trou = p.numero - numeroAttendu;
    if (trou < 1000) paquetsPerdus += trou;  // au-delà, la télécommande a redémarré
  }
  numeroAttendu = p.numero + 1;
  paquetsRecus++;
  commande = p;
  derniereCommandeMs = millis();
  commandeRecue = true;
  portEXIT_CRITICAL(&verrou);
}

void setup() {
  Serial.begin(115200);
  pinMode(BROCHE_LED, OUTPUT);
  digitalWrite(BROCHE_LED, HIGH);
  delay(1500);  // le temps que le port USB apparaisse
  Serial.println();
  Serial.println("=== Recepteur d'essai drone ===");
  if (!radioDemarrer(surReception)) {
    Serial.println("ERREUR : ESP-NOW n'a pas demarre");
    while (true) delay(1000);
  }
  Serial.printf("En ecoute sur le canal %d, adresse MAC %s\n", CANAL_WIFI, WiFi.macAddress().c_str());
}

void loop() {
  static uint32_t dernierEnvoiMs = 0, dernierAffichageMs = 0;
  static uint16_t numeroTelemetrie = 0;
  uint32_t maintenant = millis();

  portENTER_CRITICAL(&verrou);
  PaquetCommande c = commande;
  bool recue = commandeRecue;
  uint32_t age = maintenant - derniereCommandeMs;
  uint32_t recus = paquetsRecus, perdus = paquetsPerdus;
  paquetsRecus = paquetsPerdus = 0;  // cumulés ci-dessous jusqu'au prochain affichage
  portEXIT_CRITICAL(&verrou);
  static uint32_t cumulRecus = 0, cumulPerdus = 0;
  cumulRecus += recus;
  cumulPerdus += perdus;

  bool lienOk = recue && age < DELAI_PERTE_RADIO_MS;
  uint8_t etat = !lienOk ? ETAT_PERTE_RADIO : (c.arme ? ETAT_ARME : ETAT_DESARME);
  digitalWrite(BROCHE_LED, lienOk ? LOW : HIGH);

  if (maintenant - dernierEnvoiMs >= 100) {
    dernierEnvoiMs = maintenant;
    PaquetTelemetrie t = {};
    t.cle = CLE_APPAIRAGE;
    t.type = TYPE_TELEMETRIE;
    t.etat = etat;
    t.numero = numeroTelemetrie++;
    radioEnvoyer(&t, sizeof(t));
  }

  if (maintenant - dernierAffichageMs >= 500) {
    dernierAffichageMs = maintenant;
    if (!recue) {
      Serial.println("En attente de la telecommande...");
    } else {
      uint32_t total = cumulRecus + cumulPerdus;
      float qualite = total ? 100.0f * cumulRecus / total : 0;
      Serial.printf("%-12s Gaz %4u  Roulis %+4d  Tangage %+4d  Lacet %+4d  | dernier paquet il y a %lu ms, liaison %.0f %%\n",
                    nomEtat(etat), c.gaz, c.roulis, c.tangage, c.lacet, (unsigned long)age, qualite);
    }
    cumulRecus = cumulPerdus = 0;
  }
}
