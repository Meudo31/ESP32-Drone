// Copier ce fichier en config.h et adapter si besoin.
#pragma once

// Même clé et même canal que la télécommande (config.h de l'étape 1)
#define CLE_APPAIRAGE 0x5A3C91E7UL
#define CANAL_WIFI 1

// Gaz maximum autorisé sur le banc (sur 1000). Reste bas pour les premiers essais.
#define GAZ_MAX_BANC 500

// Part des manches roulis/tangage/lacet ajoutée aux gaz, pour vérifier le mélange
// (manche à droite : les moteurs gauches doivent accélérer)
#define GAIN_MANCHES_BANC 0.3f

// Tension de la LiPo 1S en dessous de laquelle on signale une batterie faible (en mV).
// Sous charge la tension chute : 3400 mV moteurs tournants laisse un peu de marge.
#define BATTERIE_FAIBLE_MV 3400

// Rapport du pont diviseur sur GPIO1 (100 kΩ + 100 kΩ = 2)
#define RAPPORT_PONT_BATTERIE 2.0f
