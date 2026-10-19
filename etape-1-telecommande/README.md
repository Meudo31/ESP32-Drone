# Étape 1 : la télécommande ESP-NOW

On construit la télécommande du drone : deux joysticks et un interrupteur d'armement sur un ESP32 DevKit, qui envoie les manches 50 fois par seconde en ESP-NOW. En face, un ESP32-C3 SuperMini sans rien de branché fait office de récepteur d'essai : il affiche ce qu'il reçoit, compte les paquets perdus et répond, ce qui allume la LED de la télécommande. C'est aussi le moment de faire un test de portée.

Le dossier contient deux programmes : `src/telecommande/` et `src/recepteur/`. La télécommande ne changera plus jusqu'à la fin du projet.

## Câblage de la télécommande (ESP32 DevKit)

| De | Vers |
| --- | --- |
| Joystick gauche, VRy (gaz) | GPIO34 |
| Joystick gauche, VRx (lacet) | GPIO35 |
| Joystick droit, VRy (tangage) | GPIO32 |
| Joystick droit, VRx (roulis) | GPIO33 |
| Joysticks, +5V | **3V3** (pas 5 V : l'ESP32 lit au maximum 3,3 V) |
| Joysticks, GND | GND |
| Interrupteur d'armement | GPIO25 et GND |

Les boutons poussoirs des joysticks (SW) ne servent pas. Les entrées sont toutes sur l'ADC1 (GPIO32 à 39), car l'ADC2 ne fonctionne pas quand la radio est active.

Le récepteur d'essai (ESP32-C3 SuperMini) n'a besoin que de son câble USB-C.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h` et change la clé d'appairage (n'importe quel nombre).
2. Branche l'ESP32 DevKit et lance `pio run -e telecommande -t upload`, puis `pio device monitor`. **Ne touche pas aux manches au démarrage** : la télécommande mesure leur position centrale.
3. Pousse le manche des gaz en butée basse puis haute et relève la valeur « brut » affichée. Reporte-les dans `GAZ_ADC_MIN` et `GAZ_ADC_MAX`, puis téléverse à nouveau.
4. Vérifie les sens, et mets `INVERSER_...` à 1 dans `config.h` pour chaque axe qui affiche le mauvais signe :
   - gaz en haut : vers 1000 ;
   - manche droit vers la droite : roulis positif ;
   - manche droit poussé vers l'avant : tangage positif ;
   - manche gauche vers la droite : lacet positif.
5. Branche le SuperMini et lance `pio run -e recepteur -t upload`, puis `pio device monitor`. Il affiche les manches, l'âge du dernier paquet et la qualité de liaison. La LED du SuperMini et la LED bleue de la télécommande s'allument quand les deux se parlent.
6. **Armement** : l'interrupteur doit être sur arrêt au démarrage de la télécommande. S'il est sur marche, le moniteur affiche « bloque » jusqu'à ce que tu le repasses sur arrêt. Le drone ne démarrera jamais les moteurs juste parce qu'on allume la télécommande.

## Idée pour la vidéo : le test de portée

Laisse le récepteur branché sur un ordinateur portable et éloigne-toi avec la télécommande alimentée par une batterie USB. Le pourcentage de liaison montre à partir de quelle distance les paquets se perdent. Si la portée est faible, essaie un autre canal (`CANAL_WIFI`, le même des deux côtés).

## Points à vérifier ensemble

- **Joysticks bruités** : si les valeurs sautent au centre, augmente `ZONE_MORTE`.
- **Brochage du DevKit** : certaines cartes 30 broches n'ont pas toutes les broches au même endroit. Vérifie les numéros GPIO imprimés sur ta carte.
