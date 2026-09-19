# pinouNDSdiag — logiciel

Suite de tests Nintendo DS du projet **Pinoudiag**. Chaque sous-dossier est un
programme homebrew (`.nds`) indépendant, compilé séparément avec devkitARM /
libnds. Certains sont écrits pour ce projet, d'autres sont des **forks** d'un
outil open source existant déjà éprouvé — pour ne pas perdre en fiabilité en
réécrivant à zéro ce qui fonctionne déjà.

## Modules

| Dossier | Origine | Teste |
|---|---|---|
| `affichage-infos/` | Écrit pour Pinoudiag | Affichage ("AFFICHAGE OK"), modèle console, batterie, langue, pseudo firmware |
| `boutons-tactile/` | Fork d'**Input Test DS** (cphx, domaine public, 2010) — voir `boutons-tactile/PROVENANCE.md` | Croix directionnelle, boutons A/B/X/Y/L/R/Start/Select, écran tactile |

## Pourquoi pas tout reprendre ailleurs ?

Deux autres outils bien connus de la communauté DS (**Diagnose** / **DiagnoSe**
et **DSdiag**) couvrent aussi batterie, rétroéclairage, WiFi et micro, et sont
considérés comme fiables par la communauté repair/revente. Mais leur code
source n'est pas publié publiquement (freeware distribué en `.nds`
seulement), donc on ne peut pas légalement le forker dans ce dépôt.

Deux options restent possibles avec eux, sans réécrire leur travail :

- Les poser tels quels (fichiers `.nds` téléchargés séparément) à côté des
  nôtres sur la carte SD / le flashcart, et les lancer depuis le menu du
  flashcart (ou TWiLight Menu++) en complément de cette suite.
- Réimplémenter nous-mêmes, dans Pinoudiag, les fonctions qu'ils couvrent et
  qu'on n'a pas encore (rétroéclairage, WiFi, micro...) — c'est la voie
  suivie pour les prochains modules de `pinouNDSdiag`.

## Prérequis pour compiler

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) avec les paquets
  `devkitARM`, `libnds` et `libmm7`/`libmm9` (maxmod) installés
- La variable d'environnement `DEVKITARM` correctement définie
  (ex : `export DEVKITARM=/opt/devkitpro/devkitARM`)
- `grit` et `mmutil` dans le `PATH` (fournis par devkitPro, nécessaires pour
  `boutons-tactile/` qui embarque une image et une musique)

## Compilation

Chaque module se compile indépendamment, depuis son propre dossier :

```sh
cd logiciel/affichage-infos && make
cd logiciel/boutons-tactile && make
```

Chaque `make` génère son propre `.nds` (`pinouNDSdiag-affichage-infos.nds`,
`pinouNDSdiag-boutons-tactile.nds`) à la racine du module concerné.

## Test

- Sur flashcart / carte SD (R4, ez-flash, TWiLight Menu++...) : copier les
  `.nds` générés sur la carte et les lancer depuis le menu de la console.
- Sur émulateur (DeSmuME, melonDS...) : ouvrir directement le fichier `.nds`.

## Prochaines étapes

- Test visuel complet des deux écrans (mires de couleur)
- Test audio (haut-parleurs / prise casque)
- Test batterie / rétroéclairage / WiFi / micro (voir section ci-dessus)
- Menu de lancement unique regroupant tous les modules de la suite
