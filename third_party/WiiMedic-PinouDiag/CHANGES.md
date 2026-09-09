# WiiMedic (PinouDiag build) — modifications par rapport à l'upstream

Ce dossier contient un fork de [WiiMedic](https://github.com/PowFPS1/WiiMedic)
par **PowFPS1**, distribué sous GNU GPL v2 (voir `LICENSE`, inchangé).

- **Base** : WiiMedic v1.3.1 (commit cloné le 2026-09-08 depuis
  `https://github.com/PowFPS1/WiiMedic`, branche par défaut).
- **But du fork** : permettre à WiiMedic d'être lancé comme addon de
  diagnostic par [PinouDiag](https://github.com/CordonBleuCosmique/Pinoudiag)
  et de lui rendre la main automatiquement à la fin, sans intervention
  manuelle. Voir `docs/return_to_loader.md` et `docs/licensing.md` dans le
  dépôt PinouDiag pour l'analyse technique et légale complète qui motive ce
  fork (Option A retenue après validation utilisateur).

## Modifications de code

Fichier modifié : `source/main.c` (seul fichier touché).

1. **Ajout des includes** `pd_types.h` et `dol_loader.h` (vendés depuis
   `src/core/` et `src/core/addon/` du dépôt PinouDiag — voir le `Makefile`
   modifié ci-dessous).
2. **Libellés de menu** : `"Exit to Homebrew Channel"` / `"Return to the
   Homebrew Channel"` renommés en `"Return to PinouDiag"` / `"Return to
   PinouDiag (analysis and report)"` — le comportement réel a changé, le
   libellé ne doit pas induire l'utilisateur en erreur.
3. **Suppression de la variable `exit_to_hbc`** — devenue inutile : les deux
   chemins de sortie (menu "Return to PinouDiag" et bouton HOME/START)
   font désormais exactement la même chose.
4. **Remplacement du bloc de sortie** : le code upstream essayait
   `WII_LaunchTitle()` sur 4 title IDs (HBC) puis, en dernier recours,
   `SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0)` (retour au System Menu). Ce
   fork appelle à la place `pd_dol_chainload(PD_LOADER_BOOT_PATH)`, qui
   recharge et saute directement vers `sd:/boot.dol` (le binaire PinouDiag
   lui-même) — même mécanisme de chainload que celui qui a chargé WiiMedic
   en premier lieu (voir `dol_loader.c`, adapté de The Homebrew Channel,
   GPLv2). `SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0)` est conservé
   uniquement comme dernier recours si le chainload échoue (fichier
   introuvable/corrompu), pour ne jamais laisser la console figée sans
   explication à l'écran.
5. Aucune autre logique de diagnostic n'est modifiée (System Info, NAND
   Health, IOS Scan, Storage Test, Controller Diagnostics, Network Test,
   Report Generator sont strictement identiques à l'upstream).

Fichier modifié : `Makefile`.

- Ajout de `../../src/core` et `../../src/core/addon` à `SOURCES`/`INCLUDES`
  pour compiler `pd_types.c` et `dol_loader.c` dans le binaire.
- Substitution générique des chemins objets (`%.c` au lieu de
  `$(SOURCES)/%.c`) pour supporter plusieurs répertoires source.
- Chemins `LIBOGC_INC`/`LIBOGC_LIB` rendus portables (`$(DEVKITPRO)/...`
  au lieu des chemins Windows codés en dur `C:/devkitPro/...` de
  l'upstream).
- `VERSION` suffixé `-pinoudiag1` et nom de zip `WiiMedic-PinouDiag_v...`
  pour ne jamais faire passer ce binaire pour une release officielle
  PowFPS1 (voir `docs/licensing.md`).

## Ce qui n'est PAS modifié

Tous les modules de diagnostic (`system_info.c`, `nand_health.c`,
`ios_check.c`, `storage_test.c`, `controller_test.c`, `network_test.c`,
`report.c`, `ui_common.c`) sont identiques à l'upstream — conformément à la
règle du brief PinouDiag "ne pas réimplémenter les diagnostics qui existent
déjà". Seul le point d'intégration (comment WiiMedic démarre et se termine)
a été touché.

## Statut de vérification

[UNVERIFIED — non compilé, REQUIRES HARDWARE TEST] Aucun toolchain
devkitPPC n'était disponible dans l'environnement qui a produit ce fork
(voir `docs/build.md` du dépôt PinouDiag). Le chainload retour en
particulier (`pd_dol_chainload`) n'a jamais été exécuté sur matériel réel —
c'est le point le plus critique à valider avant tout usage en conditions
réelles (voir `tools/test_dol/` dans le dépôt PinouDiag, prévu pour valider
ce mécanisme en isolation avant ce fork).
