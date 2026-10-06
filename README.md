# Vevor 7-in-1 (868 MHz) → ESPHome / Home Assistant

Intégration ESPHome pour la station météo **Vevor 7-in-1 (YT60231, 868 MHz, EU)**,
reçue par un module **CC1101** piloté par un **ESP32-C3 SuperMini**.

Le décodage est un portage C++ du décodeur rtl_433
[`src/devices/vevor_7in1.c`](https://github.com/merbanan/rtl_433/blob/master/src/devices/vevor_7in1.c)
(protocol 250, `FSK_PULSE_PCM`, 2-FSK ~11.26 kbaud, ±37 kHz).

## État

| Brique | État |
|---|---|
| Décodage de la charge utile (21 octets) | ✅ implémenté et testé |
| Réassemblage NRZ/PCM (impulsions → bits → trame) | ✅ implémenté et testé |
| Tests hôte sur trame de référence réelle | ✅ 40 vérifications, 0 échec |
| Composant ESPHome (`__init__.py`, `sensor.py`, liaison `remote_receiver`) | ⏳ à écrire |
| YAML cible | ✅ écrit, à valider à la compilation |
| Capture réelle depuis le matériel | ⏳ à faire (déterminera la fréquence réelle) |

## Structure

```
components/vevor_7in1/     composant externe ESPHome
  vevor_frame.{h,cpp}      protocole : préambule, offsets, somme de contrôle, champs
  vevor_pcm.{h,cpp}        réassemblage NRZ : durées d'impulsions → flux de bits → trame
esphome/vevor-7in1.yaml    configuration ESPHome cible (aucune logique métier)
tests/                     tests hôte, sans matériel
references/                source amont rtl_433 conservée pour revue
```

Séparation stricte : **aucun parsing dans le YAML**. Le YAML ne porte que le
câblage (SPI, CC1101, `remote_receiver`), les logs et la déclaration des entités.

## Protocole (rappel)

Trame de 264 bits après le préambule `AA AA CA CA 54`. 21 octets exploités :

| Octet | Champ | Traitement |
|---|---|---|
| 0 | en-tête `0xAA` | type 0, canal 0 |
| 1 | canal | `b[1] & 0x0f` |
| 2-3 | identifiant | 16 bits |
| 4 | drapeau batterie | bit 7 = batterie faible |
| 5-6 | température | `(raw - 500) * 0.1 °C` |
| 7 | humidité | % |
| 8-9 | vent moyen | `raw / 8.333` km/h |
| 10 | rafale | `raw / 1.25` km/h — **pas d'offset −1** |
| 11-12 | direction | `((b[11] & 0x0f) << 8) \| b[12]` degrés |
| 13-14 | pluie | `raw * 0.233` mm |
| 15 | UV | `(b[15] & 0x1f) - 1` |
| 16-17 | luminosité | bit 15 = multiplicateur ×10 |
| 18 | compteur TX | incrémenté à chaque émission |
| 19 | somme de contrôle | `somme(b[0..18]) & 0xff` |
| 20 | compteur TX + 1 | `b[20] == b[18] + 1` |

Les octets 8, 9, 11, 12, 13, 14, 16 et 17 subissent un **offset de −1 appliqué
octet par octet** (rebouclage `0x00 → 0xff` compris).

## Tests

Aucun matériel requis. Aucun compilateur C++ n'étant installé sur l'hôte, `zig c++`
est utilisé via le paquet pip `ziglang` (aucun privilège requis) :

```sh
uv venv ~/.venvs/cpp && uv pip install --python ~/.venvs/cpp/bin/python ziglang
tests/run_tests.sh
```

Les tests couvrent : la somme de contrôle de la trame de référence, le décodage
complet des 12 champs, la chaîne impulsions → trame, la polarité inversée, un
décalage d'horloge émetteur de 10 %, un silence en tête de capture, le rejet des
trames corrompues et l'absence de fausse détection sur du bruit.

## Hypothèses et points à confirmer

1. **Fréquence d'émission : `868.35 MHz` est une hypothèse.** La source rtl_433 ne
   documente un centre mesuré que pour la variante US (915.031 MHz), pas pour l'EU.
   La fréquence réelle sera fixée d'après une capture matérielle (balayage
   868.30 / 868.35).
2. **Liaison CC1101 → ESP32** : le CC1101 est configuré en SPI (mode *async*) et
   sort la porteuse démodulée sur `GDO0`, câblé sur GPIO3, où `remote_receiver`
   capture les fronts. Le câblage GDO0 = GPIO3 reste à confirmer sur le montage.
3. **Quantification des impulsions** : le décodeur arrondit chaque durée à
   `round(durée / période_de_bit)` comme rtl_433. Des longues impulsions très
   bruitées peuvent franchir une borne d'arrondi ; la période est ré-estimée sur
   le préambule, ce qui absorbe un décalage d'horloge émetteur.
4. La cadence d'émission (une trame toutes les 20 s) sert de base au calcul de la
   qualité de signal ; elle est documentée dans la source rtl_433.

## Licence

**GPL-2.0.** Le décodage est un portage de `vevor_7in1.c`, distribué sous GPL-2.0 :
toute redistribution de ce dépôt doit conserver cette licence.
