# Retour au loader — la question technique critique (section 4 du brief)

C'est le point de blocage principal de tout le projet. À traiter en priorité
absolue avant d'écrire la moindre ligne de POC 4/5.

## Ce que dit le code source (VERIFIED)

### 1. WiiMedic ne "retourne" jamais à son appelant

Lecture complète de `source/main.c` (WiiMedic v1.3.1) :

```c
if (exit_to_hbc) {
    if (WII_LaunchTitle(LULZ) < 0)
      if (WII_LaunchTitle(OHBC) < 0)
        if (WII_LaunchTitle(JODI) < 0)
          if (WII_LaunchTitle(HAXX) < 0)
            SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
} else {
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}
return 0;
```

Tous les chemins de sortie (menu "Exit to Homebrew Channel", bouton HOME,
START manette GC) finissent par :
- `WII_LaunchTitle()` — relance un **title NAND installé** via ES (IOS). Si ça
  réussit, le système est déjà en train de démarrer ce nouveau title : il n'y
  a plus de "retour" possible dans le process WiiMedic.
- `SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0)` — reset logiciel vers le **Wii
  System Menu**.

Le `return 0;` final est du code mort sur matériel réel : aucun des deux
chemins ci-dessus ne rend la main à l'appelant C.

**[VERIFIED] Conséquence : WiiMedic stock ne peut pas "reprendre" PinouDiag.**
Il relance soit un title NAND (qu'on n'a pas le droit d'installer — HBC est
explicitement interdit par le brief), soit il réinitialise vers le System
Menu, ce qui efface l'état de PinouDiag en mémoire et exige une réouverture
manuelle de la lettre LetterBomb.

### 2. Le chargement de DOL "loader" est un aller simple, pas un appel de sous-routine

Lecture de `channel/channelapp/source/loader_reloc.c` (HBC, source de
référence pour tout loader DOL/ELF Wii) :

- `loader_reloc()` parse le DOL/ELF, copie chaque section à son adresse
  cible (`memmove` + `DCFlushRange` + `ICInvalidateRange`), pose l'entry
  point dans `*ep`.
- `loader_exec(ep)` :
  1. `SYS_ResetSystem(SYS_SHUTDOWN, 0, 0)` — coupe les services libogc/IOS
     du loader (network, threads, IOS courant) mais **sans reset matériel
     complet**.
  2. `__exception_closeall()` — ferme les gestionnaires d'exception.
  3. Pose deux registres SDK Nintendo (bus/CPU clock) à adresse fixe.
  4. Construit un petit stub PPC (8 instructions) copié en haut de l'arène
     MEM2, qui **efface une plage mémoire** (~0x81330000–0x81800000) puis
     fait `bctr` (branchement inconditionnel, jamais un `blr`) vers l'entry
     point de l'app chargée.
  5. Appelle ce stub — qui ne revient jamais dans `loader_exec()`.

**[VERIFIED] Conséquence : le modèle Wii pour "lancer une app homebrew" est un
handoff définitif** (proche d'un `execve()` process Unix), pas un appel de
fonction dont on attend le retour. Une fois le stub exécuté, la pile, le tas
et les services du loader sont considérés perdus/non fiables.

### 3. Le "retour au loader" que d'autres projets annoncent (WiiFlow, USB Loader
GX, HBC lui-même après un jeu) fonctionne par **relance**, pas par retour

Le stub de reload de HBC (`stub/stub.c`, exécuté au reset depuis
`STUB_ADDR_MAGIC = 0x80002f00`) :

```c
void _main (void) {
    u64 titleID = MY_TITLEID;
    if (*conf_magic == STUB_MAGIC) titleID = *conf_titleID;
    reset_ios();
    es_init();
    LaunchTitle(iosver);        // recharge l'IOS demandé
    es_init();
    LaunchTitle(titleID);       // relance HBC (ou un title custom) comme title NAND
    LaunchTitle(SYSTEM_MENU);   // fallback
}
```

Ce stub vit en mémoire basse (`0x80001800`–`0x80003000`, zone que le brief
demandait explicitement d'étudier) et **survit** à `SYS_ResetSystem` parce
qu'il n'est pas dans la plage effacée par `loader_exec()` (qui efface
uniquement le haut de MEM1/MEM2, pas cette zone basse). Le System Menu, au
boot suivant, exécute ce qu'il trouve à cette adresse avant de continuer son
propre démarrage — mécanisme documenté comme "低メモリ reload stub" dans
l'écosystème Wii homebrew.

Mais ce mécanisme **relance un title NAND par ID** (`ES_Launch`/IOCTL
`IOCTL_ES_LAUNCH`). Il ne fonctionne que pour un loader **installé comme
title/channel**. PinouDiag, par construction du brief (boot direct SD via
LetterBomb, pas de channel installé, pas de HBC), n'a pas de title ID — ce
mécanisme ne s'applique donc pas tel quel.

## Ce qui reste non vérifié

- [UNVERIFIED] Le contenu exact de la page WiiBrew "Memory Map" pour la zone
  `0x80001800`–`0x80003000` n'a pas pu être lu directement (wiibrew.org
  bloqué dans ce sandbox). L'analyse ci-dessus s'appuie sur le code source
  réel de HBC, qui est une preuve plus forte qu'une page wiki, mais une
  vérification croisée directe de WiiBrew reste recommandée si l'accès
  réseau est débloqué (ou si l'utilisateur fournit une copie locale).
- [REQUIRES HARDWARE TEST] Aucun des mécanismes ci-dessus n'a été testé sur
  Wii réelle ni même compilé dans cet environnement (pas de toolchain
  devkitPPC disponible ici — voir `build.md`).

## Options d'architecture pour PinouDiag (décision à trancher avec l'utilisateur)

Le brief suppose "WiiMedic termine → PinouDiag reprend" comme un enchaînement
simple. Ce n'est **pas** ce que le code montre. Trois options réalistes :

### Option A — Fork WiiMedic (GPLv2) pour un vrai chainload aller-simple vers PinouDiag

Remplacer, dans le fork, `SYS_ResetSystem(SYS_RETURNTOMENU, ...)` /
`WII_LaunchTitle()` par un appel à une routine `dol_loader_exec()` (adaptée du
`loader_reloc.c`/`loader_exec()` de HBC, réutilisable sous GPLv2 avec mention
de licence et disponibilité des sources) qui recharge et saute directement
vers `PinouDiag/boot.elf` (ou un `boot.dol` de reprise) sur la SD. C'est
exactement le même mécanisme qu'utilise HBC pour lancer WiiMedic en premier
lieu, mais appliqué en sens inverse à la sortie de WiiMedic.

- Avantages : pas d'intervention manuelle ; conforme à l'esprit "sans
  intervention manuelle entre les étapes" (section 23) ; réutilise un
  mécanisme déjà vérifié dans le code HBC.
- Contraintes : c'est une modification de WiiMedic (GPLv2) → obligation de
  redistribuer les sources modifiées, garder la licence, documenter les
  changements (voir `licensing.md`). Le binaire distribué n'est plus le
  `boot.dol` officiel de PowFPS1 mais un fork "WiiMedic (patched for
  PinouDiag)" — à nommer et documenter clairement pour ne pas induire en
  erreur les utilisateurs de WiiMedic upstream.
- Risque technique : réécrire un chainload correct (gestion de la pile, de
  l'IOS courant, du cache I/D) est le genre d'opération qui *doit* être
  validée sur Wii réelle avant d'être qualifiée de fonctionnelle (règle
  section 22 : ne jamais déclarer "fonctionnel" sans test réel).

### Option B — Pas de reprise automatique : ré-entrée manuelle par LetterBomb

WiiMedic tourne comme aujourd'hui (stock, non modifié), écrit son rapport sur
SD/USB, puis retourne au System Menu. L'utilisateur rouvre la lettre
LetterBomb (déjà présente, pas besoin de la régénérer) pour relancer
`PinouDiag/boot.elf`, qui détecte — via le fichier de session (`PinouDiag.session`,
section 6 du brief) — que l'étape WiiMedic est déjà marquée terminée, lit
`WiiMedic_Report.txt`, et enchaîne directement sur l'analyse/rapport.

- Avantages : zéro modification de WiiMedic, zéro obligation de fork,
  zéro risque de casser le chainload PPC bas niveau.
- Contraintes : viole littéralement "sans intervention manuelle entre les
  étapes" (section 23) — il faut rouvrir le Message Board Wii une fois.
  Reste néanmoins un enchaînement très court pour le technicien (deux clics).

### Option C — WiiMedic non modifié, mais PinouDiag l'appelle en "aller simple" (comme HBC) et n'essaie pas de reprendre la main dans la même session

PinouDiag charge WiiMedic.dol avec un `loader_reloc()`/`loader_exec()` (comme
HBC), sans rien modifier côté WiiMedic. Une fois WiiMedic lancé, PinouDiag est
considéré "terminé" pour cette session boot — WiiMedic gère sa propre sortie
comme aujourd'hui (vers System Menu ou HBC si présent). Revient en pratique à
l'option B pour la reprise, mais sans même prétendre à un retour possible côté
PinouDiag.

## Recommandation

Techniquement, **l'option A est la seule qui respecte l'objectif "sans
intervention manuelle" du brief**, mais elle implique de forker et
redistribuer une version modifiée de WiiMedic sous GPLv2 — c'est une décision
produit/légale, pas seulement technique. C'est un point qui doit être validé
explicitement avant d'aller plus loin en implémentation (POC 4/5), plutôt que
tranché unilatéralement.
