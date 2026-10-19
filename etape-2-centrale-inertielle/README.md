# Étape 2 : la centrale inertielle

Le drone apprend à savoir s'il penche. Le MPU6050 mesure les accélérations (dont la gravité) et les vitesses de rotation ; un filtre complémentaire combine les deux pour obtenir le roulis et le tangage, sans la dérive du gyroscope ni le bruit de l'accéléromètre. Les courbes s'affichent en direct sur l'ordinateur.

C'est l'étape idéale pour expliquer en vidéo pourquoi un drone a besoin des deux capteurs : on montre l'angle de l'accéléromètre seul (qui tremble), celui du gyroscope seul (qui dérive), puis le mélange.

## Câblage (ESP32-C3 SuperMini)

| GY-521 | ESP32-C3 SuperMini |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO4 |
| SCL | GPIO3 |

Monte le GY-521 **composants vers le haut**, la flèche X imprimée vers l'avant du drone. S'il est tourné, règle `IMU_ROTATION` dans `config.h` (90, 180 ou 270, dans le sens horaire vu de dessus).

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`.
2. Lance `pio run -t upload`, puis `pio device monitor`. Pose la carte à plat et immobile : la LED clignote pendant la calibration du gyroscope.
3. Pour voir les courbes, installe l'extension **Teleplot** dans VS Code et connecte-la au port série à 115200 bauds. Tape `t` dans le moniteur pour passer à l'affichage texte.
4. Vérifie les signes, ils doivent être exactement ceux-là pour la suite :
   - côté droit qui descend : **roulis positif** ;
   - nez qui monte : **tangage positif** ;
   - rotation vers la droite vue de dessus : **vitLacet positive**.
5. Pose le futur drone (ou la plaque) bien à plat et tape `n` : la position est enregistrée comme « à plat » et gardée en mémoire pour l'étape 4.

## Points à vérifier ensemble

- **MPU6050 introuvable** : vérifie les fils, puis l'adresse (0x68 si la broche AD0 est en l'air ou à GND).
- **Clones** : certains modules vendus comme MPU6050 contiennent un MPU6500. Le code affiche alors un avertissement et continue, les registres utilisés sont les mêmes.
- **Réglage du filtre** : `IMU_ALPHA` proche de 1 donne une courbe plus lisse mais qui revient plus lentement à la vérité. On le réglera quand les moteurs vibreront.
