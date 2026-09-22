# pinouNDSdiag — logiciel

Suite de tests Nintendo DS du projet **Pinoudiag**. Chaque sous-dossier est un
programme homebrew (`.nds`) indépendant, compilé séparément avec devkitARM /
libnds. Certains sont écrits pour ce projet, d'autres sont des **forks** d'un
outil open source existant déjà éprouvé — pour ne pas perdre en fiabilité en
réécrivant à zéro ce qui fonctionne déjà.

## À utiliser en pratique : `orchestrateur/`

**[`orchestrateur/`](orchestrateur/README.md)** est le point d'entrée
recommandé : un seul `.nds` qui enchaîne automatiquement toutes les étapes de
diagnostic ci-dessous, écrit un rapport JSON sur la carte SD à chaque
passage, et ne s'arrête que pour les actions qu'un humain doit faire à la
place du logiciel (appuyer sur un bouton, toucher l'écran).

`affichage-infos/` et `boutons-tactile/` restent en plus, en tant que
modules indépendants — utiles pour retester un point précis isolément, ou
comme base de code pour les prochaines étapes de l'orchestrateur.

## Modules

| Dossier | Origine | Teste |
|---|---|---|
| `orchestrateur/` | Écrit pour Pinoudiag (réutilise le moteur de `boutons-tactile/`) | Enchaîne automatiquement tous les tests ci-dessous + rapport JSON |
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
  suivie pour les prochains modules de `pinouNDSdiag`, à terme intégrés
  dans `orchestrateur/`.

## Prérequis pour compiler

- [devkitPro](https://devkitpro.org/wiki/Getting_Started) avec les paquets
  `devkitARM`, `libnds`, `libfat` et `libmm7`/`libmm9` (maxmod) installés
- La variable d'environnement `DEVKITARM` correctement définie
  (ex : `export DEVKITARM=/opt/devkitpro/devkitARM`)
- `grit` et `mmutil` dans le `PATH` (fournis par devkitPro, nécessaires pour
  `boutons-tactile/` et `orchestrateur/`, qui embarquent une image et une
  musique)

## Compilation

Chaque module se compile indépendamment, depuis son propre dossier :

```sh
cd logiciel/orchestrateur   && make   # l'outil à utiliser en pratique
cd logiciel/affichage-infos && make
cd logiciel/boutons-tactile && make
```

Chaque `make` génère son propre `.nds` (`pinouNDSdiag-orchestrateur.nds`,
`pinouNDSdiag-affichage-infos.nds`, `pinouNDSdiag-boutons-tactile.nds`) à la
racine du module concerné.

## Test

- Sur flashcart / carte SD (R4, DSpico, TWiLight Menu++...) : copier les
  `.nds` générés dans le dossier de jeux de la carte (ex : `Games/` sur
  DSpico) et les lancer depuis le menu de la console. Le menu de la carte
  liste tous les `.nds` présents — rien n'empêche d'en garder plusieurs.
- Sur émulateur (DeSmuME, melonDS...) : ouvrir directement le fichier `.nds`.
  L'écriture du rapport nécessite une image de carte SD virtuelle
  correctement configurée dans l'émulateur (sinon l'orchestrateur continue
  sans bloquer, mais sans enregistrer de rapport).

## Prochaines étapes

- Ajouter l'étape WiFi à `orchestrateur/`
- Étendre le test écran (nuances de gris, coordonnées tactiles du défaut sur
  l'écran du bas) — voir `orchestrateur/README.md`
