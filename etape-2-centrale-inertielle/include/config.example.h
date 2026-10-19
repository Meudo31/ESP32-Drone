// Copier ce fichier en config.h et adapter si besoin.
#pragma once

// Orientation du GY-521 : angle (sens horaire vu de dessus) entre l'avant du drone
// et la flèche X imprimée sur le module. 0, 90, 180 ou 270.
#define IMU_ROTATION 0

// Part du gyroscope dans le filtre complémentaire (0,98 à 0,999)
#define IMU_ALPHA 0.996f
