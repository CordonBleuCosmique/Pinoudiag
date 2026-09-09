# Licences des projets réutilisés

## WiiMedic

- **Dépôt** : https://github.com/PowFPS1/WiiMedic
- **Licence** : GNU GPL v2 (fichier `LICENSE` complet, texte standard FSF)
- **Version analysée** : v1.3.1 (`meta.xml`), clonée en lecture seule le
  2026-09-08
- **Fichiers utilisés** : aucun code copié à ce stade (phase recherche
  uniquement) — PinouDiag consomme WiiMedic comme **binaire externe**
  (`boot.dol`) exécuté par chainload, pas par lien statique/dynamique. Le
  brief interdit explicitement de réimplémenter WiiMedic.
- **Modifications** : Option A retenue (voir `return_to_loader.md`) et
  **implémentée** dans `third_party/WiiMedic-PinouDiag/` (POC4/5) :
  - Licence GPLv2 conservée telle quelle (`third_party/WiiMedic-PinouDiag/LICENSE`,
    copie inchangée de l'upstream).
  - Modifications documentées précisément dans
    `third_party/WiiMedic-PinouDiag/CHANGES.md` (fichier par fichier, ligne
    par ligne).
  - Sources du fork redistribuées intégralement avec le dépôt PinouDiag
    (pas de lien externe — le code est présent dans ce dépôt).
  - Nommage distinct appliqué : `WiiMedic (PinouDiag build)`,
    version suffixée `1.3.1-pinoudiag1`, zip de release nommé
    `WiiMedic-PinouDiag_v...` (jamais `WiiMedic_v...` seul), bannière
    explicite en tête du `README.md` du fork.
  - `addons/WiiMedic/manifest.json` référence explicitement le fork et
    l'upstream (`source`, `fork`, `license`).
- **Obligations de redistribution** : la SD `PinouDiag/addons/WiiMedic/`
  contiendra un binaire GPLv2 — la structure de livraison PinouDiag devra
  inclure soit le code source correspondant, soit une offre écrite d'accès
  aux sources (art. 3 GPLv2), quelle que soit l'option retenue dans
  `return_to_loader.md`.

## The Homebrew Channel (référence pour le chainloader DOL/ELF)

- **Dépôt** : https://github.com/dhtdht020/hbc
- **Licence** : GNU GPL v2 or later (`COPYING`, et en-têtes de fichiers
  individuels, ex. `stub/stub.c` : "GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License, or (at
  your option) any later version")
- **Fichiers pertinents pour PinouDiag** : `channel/channelapp/source/loader_reloc.c`,
  `loader_reloc.h` (parseur DOL/ELF + séquence de chainload). Ce code n'est
  **pas encore copié** dans PinouDiag à ce stade (phase recherche) — s'il est
  réutilisé/adapté en implémentation, il faudra :
  - Garder les en-têtes de copyright d'origine (dhewg, marcan — voir
    `stub/stub.c`) dans les fichiers dérivés.
  - Publier le code du module `dol_loader` PinouDiag sous GPLv2 (ou
    compatible) puisqu'il dérive d'un travail GPLv2.
  - Créditer explicitement HBC/dhtdht020 comme origine dans
    `docs/build.md` et dans les commentaires du fichier adapté.
- **Note** : ce dépôt précise lui-même dans son `README.md` que le code
  publié diffère du binaire officiel distribué (protections anti-piratage
  retirées) et n'a été testé que sous l'émulateur Dolphin par ses auteurs —
  aucune garantie de fonctionnement matériel réel n'est donnée par
  l'upstream non plus.

## libogc / libfat / wiiuse / bte / devkitPPC

- Toolchain et bibliothèques standard de l'écosystème devkitPro. Licences
  usuelles de cet écosystème (libogc : mélange BSD-like/zlib selon les
  fichiers, à vérifier précisément si du code y est copié — PinouDiag
  ne fait ici que **lier** contre ces bibliothèques via le Makefile, ce qui
  n'entraîne pas d'obligation de redistribution de code PinouDiag sous une
  licence particulière). [UNVERIFIED en détail — dépôt non audité dans
  cette phase, le lien dynamique/statique standard via devkitPPC est
  cependant l'usage établi de tout l'écosystème homebrew Wii, y compris par
  WiiMedic et HBC eux-mêmes]

## Règle générale appliquée

Conformément au brief (section 18-19) : chaque réutilisation documentée ici
avant d'être effective en code, avec nom, URL, licence, version, fichiers
utilisés, modifications et obligations. "Open source" n'est jamais traité
comme "sans obligation" — GPLv2 en particulier implique redistribution du
code source pour tout binaire distribué dérivé.
