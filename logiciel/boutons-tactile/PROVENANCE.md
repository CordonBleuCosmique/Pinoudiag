# Provenance de ce module

Ce module n'est **pas** un code écrit pour Pinoudiag : c'est un fork quasi à
l'identique d'un outil homebrew existant, éprouvé depuis 2010 par la
communauté DS, plutôt qu'une réécriture qui aurait perdu sa fiabilité.

## Origine

- **Projet** : Input Test DS
- **Auteur** : cphx (Stefan Klempera)
- **Licence** : domaine public (classé "(PD)" sur la page du projet)
- **Source récupérée depuis** :
  https://sourceforge.net/projects/inputtestds/
  (archive `inputtest-ds-080210-en-src.rar`, publiée le 2010-08-02)
- **Date de récupération pour Pinoudiag** : 2026-09-19

## Fichiers repris

| Fichier ici                        | Fichier d'origine (archive)         |
|-------------------------------------|--------------------------------------|
| `source/InputTestDS.cpp`            | `InputTestDS/source/InputTestDS.cpp` |
| `gfx/logo.png`, `gfx/logo.grit`     | `InputTestDS/data/logo.png`, `logo.grit` |
| `audio/xenon.mod`                   | `InputTestDS/data/xenon.mod`         |

Fichiers de l'archive **non repris** car non pertinents (résidus d'édition
côté auteur, pas du code/des assets) : `data/Thumbs.db` (cache de vignettes
Windows), `data/SavEC1.tmp` (fichier temporaire vide), `InputTestDS.pnproj`
(fichier de projet ProjectMaker/ProNews propre à l'IDE de l'auteur, sans
utilité pour notre build).

## Modifications apportées

- Un bloc de commentaire a été ajouté en tête de `InputTestDS.cpp` pour
  référencer ce fichier — **aucune ligne de logique n'a été modifiée**.
- Le `Makefile` a été adapté pour s'intégrer à la structure de
  `pinouNDSdiag` : nom de cible (`TARGET := pinouNDSdiag-boutons-tactile`),
  dossiers renommés (`data/` → `gfx/` + `audio/` séparés, au lieu d'un seul
  dossier `data/` mélangeant image et musique). La logique de compilation
  (règles grit / mmutil / linkage) est identique à l'original.

## Ce que ce module teste

- Croix directionnelle et boutons (A, B, X, Y, L, R, Start, Select)
- Écran tactile (coordonnées brutes et calibrées)

Voir le code source pour le détail exact (écran d'accueil, à propos,
crédits — conservés tels quels).

## Utilisation dans l'orchestrateur

`../orchestrateur/` reprend le principe de lecture des boutons de ce
module (scanKeys/keysHeld), mais restructuré en liste à cocher avec
sortie définie — nécessaire pour un enchaînement automatique de tests —
au lieu de la démo libre sans fin d'origine. Le test tactile de
l'orchestrateur, lui, est une réécriture complète sans lien avec ce fork
(couverture par grille peinte au stylet). Ce dossier-ci reste la version
fidèle à l'original, non modifiée fonctionnellement.

## Pourquoi pas les autres outils trouvés (Diagnose, DSdiag) ?

Ces deux outils sont plus complets (batterie, rétroéclairage, WiFi, micro)
mais leur code source n'est pas publié publiquement à notre connaissance —
seul le binaire `.nds` circule, sans licence explicite. On ne peut donc pas
les forker légalement. Ils restent utilisables en parallèle sur la même
carte SD (voir `../README.md`), et les fonctions qu'ils couvrent seront
réimplémentées nous-mêmes dans Pinoudiag au fur et à mesure.
