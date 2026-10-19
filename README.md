# Drone ESP32

Un micro-drone quadrirotor fait maison, avec un ESP32-C3 SuperMini comme contrôleur de vol et une télécommande ESP32 qui lui parle en ESP-NOW. Il pèse autour de 60 g, sans caméra : c'est le cas le plus simple côté réglementation (voir plus bas).

Le projet avance par étapes, du plus simple au plus complet. Chaque étape est un projet PlatformIO autonome et peut faire l'objet d'une vidéo.

| Étape | Dossier | Ce que ça fait | Carte |
| --- | --- | --- | --- |
| 1 | [etape-1-telecommande](etape-1-telecommande) | Télécommande à joysticks, liaison radio ESP-NOW, test de portée | ESP32 DevKit + ESP32-C3 SuperMini |
| 2 | [etape-2-centrale-inertielle](etape-2-centrale-inertielle) | Le drone sait s'il penche : MPU6050, filtre complémentaire, courbes en direct | ESP32-C3 SuperMini |
| 3 | [etape-3-moteurs](etape-3-moteurs) | Banc d'essai des 4 moteurs **sans hélices**, armement et coupure de sécurité | ESP32-C3 SuperMini |
| 4 | [etape-4-premier-vol](etape-4-premier-vol) | Contrôleur de vol stabilisé (PID), premier vol en intérieur | ESP32-C3 SuperMini |
| Bonus | [bonus](bonus) | Télémétrie sur écran, maintien d'altitude, réglage PID par Wi-Fi, caméra | |

Le code compile avec PlatformIO mais n'a pas encore été testé sur du vrai matériel. Les valeurs de PID de l'étape 4 sont des valeurs de départ, à régler ensemble au premier vol.

## Choix techniques

- **Moteurs brushed coreless 8520 + MOSFET** plutôt que des moteurs brushless avec ESC : moins chers, plus légers, pilotables directement en PWM, et un drone de 60 g fait beaucoup moins de dégâts en cas de chute. C'est le principe de l'ESP-Drone d'Espressif.
- **ESP-NOW** plutôt que le Wi-Fi classique : pas de box, pas de connexion, latence de quelques millisecondes. Les paquets portent une clé d'appairage pour que le drone n'obéisse qu'à ta télécommande.
- **Mode stabilisé (angle)** : manches au centre, le drone revient à plat. Plus facile à piloter et à filmer que le mode acrobatique.
- **Coupure moteurs en cas de perte radio** : le drone tombe au lieu de partir tout seul. Avec un micro-drone qui vole à un ou deux mètres, c'est le choix le plus sûr.

## Liste d'achat par étape

Prix approximatifs ; chaque étape réutilise ce qui a été acheté avant. Les liens sont des recherches, pas des produits précis : compare les avis et les vendeurs.

### Étape 1 : la télécommande (environ 15 à 20 €)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| ESP32 DevKit V1 (ESP32-WROOM-32, 30 ou 38 broches) | 5 à 7 € | [Amazon](https://www.amazon.fr/s?k=esp32+devkit+v1) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=esp32+devkit+v1) |
| 2 joysticks analogiques KY-023 | 2 à 4 € | [Amazon](https://www.amazon.fr/s?k=joystick+KY-023) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=KY-023+joystick) |
| Interrupteur à levier (armement) | 1 € | [Amazon](https://www.amazon.fr/s?k=interrupteur+a+levier+miniature) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=toggle+switch+mini) |
| ESP32-C3 SuperMini, lot de 2 (une deviendra le cerveau du drone, l'autre de rechange) | 5 à 8 € | [Amazon](https://www.amazon.fr/s?k=esp32-c3+supermini) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=esp32+c3+supermini) |
| Plaque perforée, fils Dupont, petit boîtier ou planche | 3 à 5 € | |
| Alimentation de la télécommande : une batterie externe USB suffit | déjà là | |

Conseil : pour les gaz, un joystick qui revient au centre n'est pas idéal. Enlève le ressort de l'axe vertical du joystick gauche (il se démonte facilement), ou remplace cet axe par un potentiomètre à glissière de 10 kΩ.

### Étape 2 : la centrale inertielle (environ 3 €)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| Module MPU6050 GY-521 (en prendre 2, ils sont fragiles) | 2 à 4 € | [Amazon](https://www.amazon.fr/s?k=GY-521+MPU6050) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=GY-521+MPU6050) |

### Étape 3 : les moteurs (environ 30 à 40 €)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| 4 moteurs coreless 8520 3,7 V (2 horaires + 2 anti-horaires), souvent vendus avec hélices 55 ou 65 mm | 8 à 12 € | [Amazon](https://www.amazon.fr/s?k=moteur+8520+coreless) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=8520+coreless+motor+propeller) |
| Hélices de rechange 55 mm pour arbre 0,8 mm (CW + CCW) | 3 à 5 € | [AliExpress](https://www.aliexpress.com/wholesale?SearchText=55mm+propeller+0.8mm+8520) |
| 4 MOSFET logiques SI2302 ou AO3400 (boîtier SOT-23) + 4 adaptateurs SOT-23 vers DIP | 3 à 5 € | [Amazon](https://www.amazon.fr/s?k=AO3400+mosfet) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=SI2302+SOT-23) |
| 4 diodes Schottky SS14 ou 1N5819 | 1 à 2 € | [AliExpress](https://www.aliexpress.com/wholesale?SearchText=1N5819+diode) |
| Résistances : 4 × 10 kΩ, 4 × 100 Ω, 2 × 100 kΩ | 1 € | |
| 2 batteries LiPo 1S 3,7 V 600 à 800 mAh, 20C minimum, prise JST-PH 2.0 | 8 à 14 € | [Amazon](https://www.amazon.fr/s?k=lipo+1s+3.7v+600mah) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=lipo+1s+3.7v+600mah+20c) |
| Chargeur LiPo 1S USB (prise adaptée à la batterie) | 3 à 6 € | [Amazon](https://www.amazon.fr/s?k=chargeur+lipo+1s+usb) |
| Condensateur 470 µF 6,3 V faible ESR + condensateur 100 nF | moins d'1 € | |
| Mini interrupteur à glissière (alimentation de la carte) | moins d'1 € | |
| Sac ignifuge pour LiPo | 5 à 8 € | [Amazon](https://www.amazon.fr/s?k=sac+ignifuge+lipo) |

### Étape 4 : le premier vol (environ 8 à 15 €)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| Cadre de micro-drone pour moteurs 8520, idéalement avec protections d'hélices (à imprimer en 3D ou à acheter) | 0 à 10 € | [Printables](https://www.printables.com/search/models?q=8520%20quadcopter%20frame) · [AliExpress](https://www.aliexpress.com/wholesale?SearchText=8520+frame+quadcopter) |
| Fil silicone 26 ou 28 AWG, gaine thermo | 3 € | |
| Mousse double face (anti-vibrations sous le MPU6050), élastiques | 2 € | |

**Total : environ 60 à 80 €** pour un drone complet et sa télécommande, sans compter le fer à souder.

Masse estimée : cadre 10 g, moteurs 4 × 5 g, batterie 15 à 18 g, électronique et fils 10 g, soit environ 55 à 60 g. Quatre 8520 avec hélices de 55 mm donnent ensemble à peu près le double, ce qui laisse de la marge pour voler.

## Sécurité

À rappeler dans chaque vidéo.

- **Jamais d'hélices** sur le banc (étape 3), pendant un téléversement ou une modification du code. On monte les hélices seulement pour voler.
- **USB et LiPo ne doivent jamais alimenter la carte en même temps** : l'interrupteur d'alimentation de la carte sert à ça. Interrupteur sur arrêt avant de brancher le câble USB.
- **Batteries LiPo** : charge surveillée dans le sac ignifuge, jamais en dessous de 3,0 V, jamais une batterie gonflée ou abîmée, débranchée après chaque vol.
- **Premier vol** en intérieur, pièce dégagée, protections d'hélices montées, personnes et animaux à distance, lunettes. Pour le tout premier décollage, attache le drone au sol avec un fil de pêche d'un mètre.
- **Coupure de sécurité** : en cas de perte radio, de chute ou de problème du capteur, les moteurs s'arrêtent et le drone tombe. Reste bas tant que les réglages ne sont pas validés.
- Les moteurs coreless chauffent : pas de plein gaz prolongé sur le banc.

## Réglementation drone en France

Ce qui suit résume les règles de la catégorie « Ouverte » (règlement européen 2019/947, appliqué par la DGAC). Les règles évoluent : vérifie les sources officielles avant de tourner et avant chaque nouvelle vidéo.

- **Ce drone pèse moins de 250 g et n'a pas de caméra.** C'est le cas le plus léger : d'après la notice officielle, l'enregistrement de l'exploitant sur AlphaTango n'est obligatoire qu'au-delà de 250 g ou dès que l'appareil porte un capteur capable d'enregistrer des données personnelles (caméra, micro).
- **Ajouter une caméra change tout** : il faut alors s'enregistrer comme exploitant sur AlphaTango et coller son numéro sur le drone. À dire clairement dans une vidéo bonus avec caméra.
- **Formation** : la formation en ligne gratuite de la DGAC (sur AlphaTango) est conseillée dans tous les cas, et obligatoire au-dessus de 250 g. Passe-la avant de voler dehors, ça se montre bien en vidéo.
- **Règles de vol dehors** : 120 m de hauteur maximum, toujours à vue, jamais au-dessus d'un rassemblement de personnes, pas dans les zones interdites (aéroports, sites sensibles, certaines villes) : vérifier la carte Géoportail des restrictions avant chaque vol. Sur un terrain privé, il faut l'accord du propriétaire. Pour le vol de nuit, vérifier les conditions en vigueur auprès de la DGAC avant de le montrer.
- **Vie privée** : ne pas filmer ni survoler des personnes sans leur accord.
- **Assurance** : vérifier que ta responsabilité civile couvre l'usage d'un drone de loisir.
- **En intérieur**, dans un local fermé, on est en principe hors de l'espace aérien : c'est l'endroit idéal pour les essais et le tournage. À confirmer dans le guide officiel.
- **Pour le tournage**, filme le drone avec une caméra ou un téléphone extérieur : le drone reste sans caméra, donc sans enregistrement.
- **Si le drone dépasse 250 g** (gros cadre, grosse batterie), il passe dans la sous-catégorie A3 : loin des habitations et des personnes, enregistrement et formation obligatoires. Pèse-le sur une balance de cuisine et montre-le à l'écran.

Sources à consulter :
- [AlphaTango](https://alphatango.aviation-civile.gouv.fr) : enregistrement et formation en ligne
- [Carte des restrictions pour drones de loisir (Géoportail)](https://www.geoportail.gouv.fr/donnees/restrictions-uas-categorie-ouverte-et-aeromodelisme)
- [Notice officielle de la préfecture d'Indre-et-Loire sur les aéronefs sans équipage](https://www.indre-et-loire.gouv.fr/index.php/contenu/telechargement/40881/288412/file/Notice_Aeronef_Sans_Equipage_a_Bord.pdf)

## Mise en route

Chaque dossier a son propre README avec le câblage et la mise en route. Dans tous les cas : copier `include/config.example.h` en `include/config.h`, mettre **la même clé d'appairage** partout, puis `pio run -t upload` depuis le dossier de l'étape. Le fichier `config.h` est exclu du dépôt.

Les fichiers `protocole.h`, `mpu6050.h` et `moteurs.h` sont recopiés à l'identique dans les étapes qui en ont besoin, pour que chaque dossier reste autonome. Si tu en modifies un, recopie-le partout.
