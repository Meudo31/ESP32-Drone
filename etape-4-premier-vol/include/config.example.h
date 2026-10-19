// Copier ce fichier en config.h et adapter si besoin.
// Les valeurs de PID ci-dessous sont des valeurs de DÉPART pour un micro-drone
// à moteurs 8520 : elles se règlent sur ton drone (voir le README).
#pragma once

// Même clé et même canal que la télécommande (config.h de l'étape 1)
#define CLE_APPAIRAGE 0x5A3C91E7UL
#define CANAL_WIFI 1

// Centrale inertielle (mêmes valeurs qu'à l'étape 2)
#define IMU_ROTATION 0
#define IMU_ALPHA 0.996f

// ---- Pilotage ----
#define ANGLE_MAX          25.0f   // inclinaison maximale demandée par le manche, en degrés
#define VITESSE_LACET_MAX 150.0f   // rotation maximale demandée par le manche, en °/s

// Boucle d'angle : degrés d'erreur -> vitesse de rotation voulue (°/s par degré)
#define KP_ANGLE 4.0f

// Boucle de vitesse (sortie en unités de gaz sur 1000)
#define KP_ROULIS  0.60f
#define KI_ROULIS  0.80f
#define KD_ROULIS  0.004f
#define KP_TANGAGE 0.60f
#define KI_TANGAGE 0.80f
#define KD_TANGAGE 0.004f
#define KP_LACET   1.50f
#define KI_LACET   0.50f
#define KD_LACET   0.0f
#define LIMITE_INTEGRALE 120.0f

// ---- Gaz ----
#define GAZ_RALENTI   60   // les hélices tournent doucement une fois armé : signe visible que c'est armé
#define GAZ_DECOLLAGE 120  // en dessous, pas de correction (évite que le drone "lutte" posé au sol)
#define GAZ_MAX       850  // garde de la marge aux corrections

// ---- Sécurités ----
#define ANGLE_CHUTE         70.0f  // au-delà, on considère que le drone est tombé : coupure
#define ANGLE_ARMEMENT_MAX  15.0f  // on n'arme que si le drone est à peu près à plat
#define BATTERIE_FAIBLE_MV  3400   // alerte batterie faible (sous charge)
#define RAPPORT_PONT_BATTERIE 2.0f // pont 100 kΩ + 100 kΩ sur GPIO1
