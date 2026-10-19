// Copier ce fichier en config.h et remplir les valeurs.
// La télécommande et le drone doivent avoir la même clé et le même canal.
#pragma once

// Clé d'appairage : un nombre quelconque, à changer pour que ton drone
// n'obéisse qu'à ta télécommande. Garde la même dans toutes les étapes.
#define CLE_APPAIRAGE 0x5A3C91E7UL

// Canal Wi-Fi utilisé par ESP-NOW (1 à 13). Change-le si la liaison est mauvaise.
#define CANAL_WIFI 1

// ---- Télécommande uniquement ----

// Valeurs brutes (0 à 4095) lues sur le manche des gaz en butée basse et haute.
// Le moniteur série de la télécommande les affiche : relève-les et reporte-les ici.
#define GAZ_ADC_MIN 150
#define GAZ_ADC_MAX 3950

// Zone morte autour du centre des manches à ressort, en pas d'ADC
#define ZONE_MORTE 80

// Mets 1 pour inverser un axe si le moniteur affiche le mauvais signe
#define INVERSER_GAZ     0
#define INVERSER_ROULIS  0
#define INVERSER_TANGAGE 0
#define INVERSER_LACET   0
