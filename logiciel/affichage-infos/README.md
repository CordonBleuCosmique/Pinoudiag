# pinouNDSdiag — affichage-infos

Premier programme de test du projet **Pinoudiag** pour Nintendo DS. Écrit
pour ce projet (pas un fork) — voir `../boutons-tactile/` pour le module de
test boutons/tactile, qui lui est un fork d'un outil existant.

Ce logiciel est une homebrew Nintendo DS (`.nds`) écrite en C avec **libnds**
(toolchain devkitARM / devkitPro). Il s'agit de la brique de départ de la
suite de diagnostic : elle sert à vérifier rapidement qu'une console reçue
pour réparation affiche bien à l'écran, et remonte les informations console
disponibles.

## Fonctionnement actuel

Au lancement, le programme :

1. Initialise la console texte (`consoleDemoInit`) et affiche **"AFFICHAGE OK"**
   — si ce texte est lisible à l'écran, l'affichage (écran + rétroéclairage
   côté console texte) fonctionne.
2. Affiche les informations disponibles sur la console testée :
   - Modèle détecté (Nintendo DSi vs DS / DS Lite, via `isDSiMode()`)
   - Niveau de batterie (0 à 15, + indicateur de charge)
   - Langue configurée dans le firmware
   - Pseudo (nom) configuré dans le firmware, si présent
3. Attend l'appui sur **START** pour quitter.

## Prérequis pour compiler

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) avec le paquet
  `devkitARM` et la librairie `libnds` installés
- La variable d'environnement `DEVKITARM` correctement définie
  (ex : `export DEVKITARM=/opt/devkitpro/devkitARM`)

## Compilation

```sh
cd logiciel/affichage-infos
make
```

Cela génère `pinouNDSdiag-affichage-infos.nds` à la racine de ce dossier.

## Test

- Sur un flashcart / carte SD (R4, ez-flash, TWiLight Menu++...) : copier
  `pinouNDSdiag-affichage-infos.nds` sur la carte et le lancer depuis le menu
  de la console.
- Sur émulateur (DeSmuME, melonDS...) : ouvrir directement le fichier `.nds`.

## Prochaines étapes

- Test visuel complet des deux écrans (mires de couleur, pas seulement le
  texte de la console)
- Test audio (haut-parleurs / prise casque)
- Test batterie / rétroéclairage / WiFi / micro
- Menu de lancement unique regroupant tous les modules (cf. `../README.md`)
