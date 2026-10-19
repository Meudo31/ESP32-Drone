# Étape 4 : le contrôleur de vol et le premier vol

Tout se rejoint : centrale inertielle de l'étape 2, moteurs de l'étape 3, télécommande de l'étape 1. La boucle de vol tourne 250 fois par seconde en mode stabilisé : les manches donnent un angle voulu, un premier régulateur transforme l'erreur d'angle en vitesse de rotation voulue, un PID par axe transforme l'erreur de vitesse en correction des moteurs.

## Câblage

Celui des étapes 2 et 3 réunies, monté sur le cadre :

- le SuperMini et le GY-521 au centre, le GY-521 posé sur de la mousse double face (les vibrations des moteurs faussent les mesures), flèche X vers l'avant ;
- les moteurs aux mêmes places qu'à l'étape 3, avec les hélices du bon sens : hélices horaires sur M1 et M3, anti-horaires sur M2 et M4 ;
- la LiPo sous le cadre, centrée, tenue par un élastique.

## Sécurités du programme

- Armement seulement avec les gaz en bas, le drone à plat (moins de 15°) et l'interrupteur repassé par arrêt.
- Une fois armé, les hélices tournent doucement (`GAZ_RALENTI`) : c'est le signe visible que le drone est armé.
- Coupure immédiate des moteurs en cas de perte radio (300 ms sans paquet), de chute (plus de 70° d'inclinaison) ou de MPU6050 qui ne répond plus.
- LED : éteinte = désarmé, allumée = armé, clignotement lent en vol = batterie faible, clignotement rapide = coupure ou armement refusé.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h` (même clé que la télécommande, même `IMU_ROTATION` qu'à l'étape 2).
2. Téléverse **sans hélices**, interrupteur d'alimentation sur arrêt, puis `pio device monitor`. Le drone calibre son gyroscope au démarrage : pose-le à plat sans le toucher.
3. Si le trim de l'étape 2 n'a pas été fait sur le drone monté, pose-le à plat et tape `n`.
4. **Test des corrections, toujours sans hélices** : arme, monte les gaz à mi-course, puis penche le drone à la main.
   - Penche le côté droit vers le bas : les moteurs droits (M2, M3) doivent accélérer pour le redresser.
   - Lève le nez : les moteurs arrière (M3, M4) doivent accélérer.
   - Tourne-le vers la droite : M1 et M3 (horaires) doivent accélérer.
   Si un axe corrige dans le mauvais sens, ne vole pas : on cherche la cause ensemble (orientation du MPU6050 ou position des moteurs).
5. Monte les hélices, débranche l'USB, mets l'interrupteur d'alimentation sur marche.
6. **Premier vol attaché**, en intérieur : un fil de pêche d'un mètre entre le drone et un poids au sol. Arme, monte doucement les gaz jusqu'à ce que le drone s'allège, et observe.

## Réglage des PID (valeurs de départ dans `config.h`)

Procède axe par axe, roulis et tangage ensemble car le drone est symétrique :

1. Mets `KI` et `KD` à 0. Augmente `KP` jusqu'à ce que le drone oscille vite autour de l'axe, puis redescends d'un tiers.
2. Augmente `KD` petit à petit pour calmer les dépassements. Trop de `KD` fait chauffer les moteurs et produit un bruit aigu.
3. Augmente `KI` jusqu'à ce que le drone ne dérive plus lentement d'un côté.
4. Si le drone revient trop mollement à plat, augmente `KP_ANGLE` ; s'il oscille lentement, baisse-le.

Le moniteur série affiche les angles, les sorties moteurs et la durée maximale d'un tour de boucle : elle doit rester bien en dessous de 4000 µs.

## Points à vérifier ensemble

- **Le drone tourne sur lui-même au décollage** : hélices inversées sur une diagonale, ou `KP_LACET` trop faible.
- **Il part toujours du même côté** : refais le trim (`n`) à plat, vérifie le centrage de la batterie.
- **Vibrations** : si les angles tremblent moteurs en marche, monte `IMU_ALPHA` (0,998) et améliore la mousse sous le MPU6050.
- **Autonomie** : avec une LiPo de 600 mAh, compte quelques minutes. Atterris dès que la LED clignote lentement.
