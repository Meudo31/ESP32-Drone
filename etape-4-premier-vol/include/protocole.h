// Protocole radio ESP-NOW entre la télécommande et le drone.
// Ce fichier est identique dans les étapes 1, 3 et 4 : si tu le modifies, copie-le partout.
//
// Les paquets partent en diffusion (adresse FF:FF:FF:FF:FF:FF), donc pas d'adresse MAC à
// configurer. Chaque paquet porte la clé d'appairage de config.h : le drone ignore tout
// paquet qui n'a pas la même clé (une autre télécommande ESP-NOW à côté, par exemple).
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>

#ifndef CANAL_WIFI
#define CANAL_WIFI 1
#endif

enum TypePaquet : uint8_t {
  TYPE_COMMANDE   = 0xC1,  // télécommande -> drone
  TYPE_TELEMETRIE = 0xD1,  // drone -> télécommande
};

enum EtatDrone : uint8_t {
  ETAT_DESARME     = 0,
  ETAT_ARME        = 1,
  ETAT_PERTE_RADIO = 2,  // plus de paquet reçu : moteurs coupés
  ETAT_CHUTE       = 3,  // angle trop grand : moteurs coupés
  ETAT_CALIBRATION = 4,
  ETAT_REFUS       = 5,  // armement refusé (gaz pas à zéro ou drone pas à plat)
};

// Bits du champ "alertes" de la télémétrie
const uint8_t ALERTE_BATTERIE_FAIBLE = 0x01;

// Conventions des manches (valeurs envoyées par la télécommande) :
//   gaz     : 0 à 1000
//   roulis  : -500 à +500, positif = manche à droite = le drone penche à droite
//   tangage : -500 à +500, positif = manche poussé = le drone pique du nez et avance
//   lacet   : -500 à +500, positif = manche à droite = le nez tourne à droite
struct __attribute__((packed)) PaquetCommande {
  uint32_t cle;
  uint8_t  type;      // TYPE_COMMANDE
  uint8_t  arme;      // 1 = interrupteur d'armement sur marche
  uint16_t gaz;
  int16_t  roulis;
  int16_t  tangage;
  int16_t  lacet;
  uint16_t numero;    // incrémenté à chaque paquet, sert à compter les pertes
};

struct __attribute__((packed)) PaquetTelemetrie {
  uint32_t cle;
  uint8_t  type;      // TYPE_TELEMETRIE
  uint8_t  etat;      // EtatDrone
  uint8_t  alertes;   // bits ALERTE_*
  uint16_t batterieMv;
  int16_t  roulis10;  // angle mesuré en dixièmes de degré
  int16_t  tangage10;
  uint16_t numero;
};

inline const char *nomEtat(uint8_t etat) {
  switch (etat) {
    case ETAT_DESARME:     return "desarme";
    case ETAT_ARME:        return "ARME";
    case ETAT_PERTE_RADIO: return "PERTE RADIO";
    case ETAT_CHUTE:       return "CHUTE";
    case ETAT_CALIBRATION: return "calibration";
    case ETAT_REFUS:       return "armement refuse";
    default:               return "?";
  }
}

// ---- Couche radio : démarrage, envoi, réception ----

using RecepteurRadio = void (*)(const uint8_t *donnees, int longueur);
static RecepteurRadio recepteurRadio = nullptr;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void rappelReceptionRadio(const esp_now_recv_info_t *, const uint8_t *donnees, int longueur) {
#else
static void rappelReceptionRadio(const uint8_t *, const uint8_t *donnees, int longueur) {
#endif
  if (recepteurRadio) recepteurRadio(donnees, longueur);
}

inline bool radioDemarrer(RecepteurRadio recepteur) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;

  esp_now_peer_info_t diffusion = {};
  memset(diffusion.peer_addr, 0xFF, 6);
  diffusion.channel = CANAL_WIFI;
  diffusion.ifidx = WIFI_IF_STA;
  diffusion.encrypt = false;
  if (esp_now_add_peer(&diffusion) != ESP_OK) return false;

  recepteurRadio = recepteur;
  esp_now_register_recv_cb(rappelReceptionRadio);
  return true;
}

inline bool radioEnvoyer(const void *paquet, size_t taille) {
  static const uint8_t DIFFUSION[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return esp_now_send(DIFFUSION, (const uint8_t *)paquet, taille) == ESP_OK;
}
