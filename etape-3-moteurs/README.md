# Étape 3 : le banc d'essai des moteurs (sans hélices)

On câble l'électronique de puissance : la LiPo, quatre MOSFET et quatre moteurs coreless. Le drone obéit à la télécommande de l'étape 1, mais sans stabilisation : mêmes gaz pour les quatre moteurs, plus une petite part des manches pour vérifier que le mélange part dans le bon sens. On vérifie aussi le sens de rotation de chaque moteur et la tenue de la batterie.

**Pas d'hélices pendant toute cette étape.**

## Câblage

Chaque moteur a son propre MOSFET (SI2302 ou AO3400), monté ainsi :

| Composant | Branchement |
| --- | --- |
| Grille (G) du MOSFET | broche GPIO du moteur à travers une résistance de 100 Ω, et à GND par une résistance de 10 kΩ |
| Source (S) | GND |
| Drain (D) | fil négatif du moteur |
| Fil positif du moteur | + de la LiPo |
| Diode Schottky | aux bornes du moteur, cathode (trait) côté + batterie |

La résistance de 10 kΩ garde les moteurs à l'arrêt tant que l'ESP32 démarre.

| Moteur | Position | Sens de rotation (vu de dessus) | GPIO |
| --- | --- | --- | --- |
| M1 | avant gauche | horaire | GPIO5 |
| M2 | avant droit | anti-horaire | GPIO6 |
| M3 | arrière droit | horaire | GPIO7 |
| M4 | arrière gauche | anti-horaire | GPIO10 |

Alimentation et mesure de batterie :

| De | Vers |
| --- | --- |
| + LiPo | interrupteur d'alimentation, puis broche **5V** du SuperMini |
| - LiPo | GND |
| Condensateur 470 µF + 100 nF | entre + et - LiPo, au plus près de la carte |
| + LiPo | 100 kΩ, puis GPIO1, puis 100 kΩ, puis GND (pont diviseur) |

Le MPU6050 de l'étape 2 peut rester branché, il ne sert pas ici.

**Ne branche jamais le câble USB quand l'interrupteur d'alimentation est sur marche.** Pour les essais avec le moniteur série, coupe l'interrupteur : la carte est alimentée par l'USB, les moteurs par la LiPo, avec la masse commune.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`, avec la même clé et le même canal que la télécommande.
2. Lance `pio run -t upload`, puis `pio device monitor`.
3. Avec la télécommande éteinte ou désarmée, tape `1`, `2`, `3` puis `4` : chaque moteur tourne doucement 1,5 s. Vérifie qu'il est à la bonne place et qu'il tourne dans le bon sens (un bout de scotch sur l'arbre aide à voir). Pour un moteur brushed, inverser ses deux fils inverse son sens.
4. Allume la télécommande, gaz en bas, puis interrupteur d'armement sur marche : la LED du drone s'allume. Monte doucement les gaz : les quatre moteurs accélèrent ensemble.
5. Gaz à mi-course, vérifie le mélange :
   - manche droit vers la droite : M1 et M4 (côté gauche) accélèrent ;
   - manche droit poussé : M3 et M4 (arrière) accélèrent ;
   - manche gauche vers la droite : M2 et M4 (anti-horaires) accélèrent.
6. Coupe la télécommande moteurs en marche : les moteurs doivent s'arrêter en moins d'un tiers de seconde et la LED clignoter. Il faut repasser l'interrupteur sur arrêt pour réarmer.

## Points à vérifier ensemble

- **Redémarrages quand les moteurs accélèrent** (le moniteur affiche « Brownout ») : la tension de la LiPo chute trop. Ajoute un condensateur plus gros et raccourcis les fils de puissance.
- **Tension sous charge** : le moniteur affiche la tension batterie. Note la chute à mi-gaz : ça servira à régler `BATTERIE_FAIBLE_MV`.
- **MOSFET qui chauffent** : vérifie que c'est bien un modèle « logic level » (SI2302, AO3400), qui conduit franchement dès 3,3 V sur la grille.
