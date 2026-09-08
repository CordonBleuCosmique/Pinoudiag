# Architecture Wii — éléments pertinents

Compilation des faits d'architecture rencontrés en lisant le code WiiMedic et
HBC ([VERIFIED] sauf mention contraire). Ce n'est pas un cours complet sur la
Wii, seulement ce qui conditionne des décisions PinouDiag.

## CPU / GPU (constantes affichées par WiiMedic, non vérifiées indépendamment
mais faisant consensus dans tout l'écosystème homebrew)

- CPU : "Broadway", IBM PowerPC 750CL, 729 MHz
- GPU : "Hollywood" (ATI/AMD), 243 MHz
- MEM1 : 24 Mo (fixe) ; MEM2 : 64 Mo (fixe) — tailles totales fixes, seule la
  taille *libre* de chaque arène varie (`SYS_GetArena1Size()`/`SYS_GetArena2Size()`)

## IOS

- Le système Wii exécute un micro-noyau **IOS** (rechargeable) séparé du
  code PowerPC applicatif, qui gère ES (Ticket/Title management), ISFS
  (accès NAND), USB, réseau, crypto.
- `IOS_GetVersion()`/`IOS_GetRevision()` lisent l'IOS **actif**, pas une
  propriété statique de la console — un titre/binaire peut demander un
  rechargement d'IOS différent avant de s'exécuter (c'est ce que fait le
  stub HBC via `LaunchTitle(0x0000000100000000 | iosver)` avant de relancer
  le canal cible).
- WiiMedic ne recharge jamais l'IOS (voir `wiimedic.md`) — tourne sous
  l'IOS déjà actif au moment de son lancement.

## ES (Effective System / Espresso — gestion des titres)

- `ES_GetDeviceID`, `ES_GetStoredTMDSize`/`ES_GetStoredTMD`,
  `ES_GetBoot2Version` : API d'accès aux tickets/TMD stockés, utilisée par
  WiiMedic pour détecter Priiloader (en comparant le contenu de boot
  actuellement enregistré pour le System Menu) et par HBC pour relancer des
  titres (`IOCTL_ES_LAUNCH` via `ios_ioctlvreboot`).
- Lancer un titre (`WII_LaunchTitle`/`ios_ioctlvreboot(IOCTL_ES_LAUNCH)`)
  est un point de non-retour pour le process appelant — voir
  `return_to_loader.md`.

## OTP / registres Hollywood bas niveau

- `HW_REG_BASE = 0xCD000000` : base des registres mappés Hollywood.
  `AHBPROT` (offset `0x064`) doit être entièrement déverrouillé
  (`0xFFFFFFFF`) pour pouvoir lire l'OTP (clés/hash boot1) — c'est l'état
  normal sous du code homebrew qui a déjà atteint l'exécution native (le
  déverrouillage AHBPROT fait partie de ce que l'exploit d'entrée, ici
  LetterBomb, met en place avant même de lancer `boot.elf`).
- Utilisé par WiiMedic pour identifier la révision boot1 (a/b exploitables,
  c/d non) via hash SHA-1 comparé à des constantes connues.

## Ce que ça implique pour PinouDiag

- PinouDiag hérite de l'état IOS/AHBPROT laissé par LetterBomb au moment où
  `boot.elf` démarre — aucune initialisation de sécurité supplémentaire à
  faire de ce côté.
- Si PinouDiag a besoin d'un IOS spécifique pour une opération (ex. accès
  NAND fiable pour son propre fingerprint), il doit le recharger
  explicitement via le même mécanisme que le stub HBC — mais cela implique
  un **rechargement IOS = relance implicite**, à séquencer soigneusement
  par rapport au chainload vers WiiMedic (idéalement : PinouDiag fait tout
  ce dont il a besoin niveau IOS *avant* de charger WiiMedic, puisqu'après
  le chainload il ne contrôle plus l'exécution — voir `return_to_loader.md`).

## Statut

[VERIFIED] pour les API et leur usage observé dans WiiMedic/HBC.
[UNVERIFIED] pour les détails d'architecture non rencontrés dans le code
(ex. carte mémoire complète, détails du pipeline GPU) — non nécessaires à ce
stade du projet.
