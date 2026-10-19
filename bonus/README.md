# Bonus : pour aller plus loin

Des idées de vidéos de suite, une fois que le drone vole. Pas de code ici pour l'instant : on le fera quand le drone de l'étape 4 sera réglé.

## Écran de télémétrie sur la télécommande

Un petit écran OLED SSD1306 (I2C, 2 à 3 €) sur la télécommande affiche la tension de la batterie du drone, son état et son inclinaison. Tout est déjà envoyé par le drone dans le paquet de télémétrie : il ne reste qu'à l'afficher. On peut ajouter un buzzer actif (1 €) qui sonne en cas de batterie faible ou de perte radio.

## Maintien d'altitude

Un capteur de distance laser VL53L1X (4 à 6 €) sous le drone mesure la hauteur jusqu'à 3 ou 4 m en intérieur. Un PID de plus sur les gaz, et le drone tient sa hauteur tout seul. Un baromètre BMP280 marche aussi, mais il est moins précis à faible hauteur.

## Réglage des PID depuis le téléphone

Le drone ouvre un point d'accès Wi-Fi au sol (moteurs désarmés) avec une page web qui permet de changer les PID et de les garder en mémoire, sans recompiler. Ça fait gagner beaucoup de temps et ça se filme bien.

## Mode acrobatique

Une seconde position de l'interrupteur (ou un bouton de joystick) qui court-circuite la boucle d'angle : les manches commandent directement les vitesses de rotation. Pour les loopings, et pour les pilotes déjà à l'aise.

## Caméra

Une caméra embarquée fait passer le drone dans le cas « capteur de données personnelles » : **enregistrement de l'exploitant sur AlphaTango obligatoire** et numéro collé sur le drone, même sous 250 g. À expliquer dans la vidéo. Le poids d'une ESP32-CAM et de sa consommation demandera aussi une batterie plus grosse et sans doute des moteurs plus puissants.

## Portée

ESP-NOW propose un mode longue portée (Wi-Fi LR) qui augmente la distance au prix du débit. À activer des deux côtés avec `esp_wifi_set_protocol`. Rappel : même avec une meilleure portée, le drone doit toujours rester en vue.
