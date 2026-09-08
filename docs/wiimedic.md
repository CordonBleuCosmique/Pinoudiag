# Analyse de WiiMedic (addon V1)

Dépôt : https://github.com/PowFPS1/WiiMedic — clone local
`/home/user/powfps1/wiimedic` (lecture seule, anonyme, HEAD au moment du
clone). Toute cette fiche est [VERIFIED] par lecture directe du code, pas du
seul README (conformément à la consigne du brief).

## Identité

- **Nom** : WiiMedic — "Wii System Diagnostic & Health Monitor"
- **Version** au moment du clone : v1.3.1 (`meta.xml`, `Makefile` dit encore
  `VERSION := 1.3.0` — incohérence mineure entre les deux fichiers, à noter
  pour le `manifest.json` PinouDiag : préférer `meta.xml` comme source de
  vérité, plus récent d'après le changelog du README)
- **Auteur** : PowFPS1
- **Licence** : **GNU GPL v2** (fichier `LICENSE` complet à la racine)

## Dépendances de build

`Makefile` : `include $(DEVKITPPC)/wii_rules`, lie contre
`-lwiiuse -lbte -lfat -logc -lm`. Chemins codés en dur en plus des variables
(`C:/devkitPro/libogc/include`, `C:/devkitPro/libogc/lib/wii` — Windows,
probablement à adapter pour build Linux). Cible finale : `boot.dol` (via
`elf2dol`).

- devkitPPC (toolchain GCC PowerPC)
- libogc **3.0.0+** (annoncé dans le README ; le Makefile ne vérifie pas la
  version)
- libfat (FAT SD/USB)
- wiiuse + bte (Wiimote/Bluetooth)

## IOS

- Aucune vérification/forçage d'IOS au démarrage dans le code (`main.c`
  n'appelle aucune fonction de rechargement d'IOS).
- Le README recommande IOS58 ("best NAND/ES access") mais dit explicitement
  que "most IOS versions work fine" — donc pas de dépendance stricte.
- `system_info.c` lit l'IOS courant (`IOS_GetVersion()/IOS_GetRevision()`)
  uniquement pour l'afficher, jamais pour le changer.
- **Conséquence pour PinouDiag** : pas besoin de recharger d'IOS spécifique
  avant de lancer WiiMedic — il tourne sous l'IOS déjà actif au moment du
  chainload. À confirmer sur matériel réel que l'IOS chargé par LetterBomb
  (généralement l'IOS du System Menu) suffit aux modules WiiMedic qui font
  de l'ISFS (NAND health, priiloader detection).

## Dépendance au Homebrew Channel

- **Pas de vérification de présence de HBC au démarrage.** Rien dans
  `main.c`/ailleurs ne bloque ou n'échoue si HBC n'est pas installé.
- HBC n'intervient que côté **sortie** (menu "Exit to Homebrew Channel" ou
  bouton HOME/START) : `WII_LaunchTitle()` sur 4 title IDs candidats, avec
  repli sur `SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0)` si aucun n'est
  installé.
- **Conclusion : lancement direct comme DOL (sans HBC installé) est
  possible** — cohérent avec la contrainte du brief "Pas de Homebrew
  Channel à installer". Voir `return_to_loader.md` pour la conséquence
  négative : la sortie de WiiMedic ne "revient" jamais à l'appelant.

## Stockage — SD/USB

- Chemins en dur : `sd:/WiiMedic_Report.txt` et `usb:/WiiMedic_Report.txt`
  (`report.c`). Priorité SD puis USB si SD indisponible en écriture.
- Gestion de rapport existant : propose overwrite / garder les deux
  (numérotation `WiiMedic_Report_2.txt` etc., jusqu'à 99) / annuler.
- `fatInitDefault()` appelé une seule fois au démarrage (`main.c`) — monte
  les devices FAT (SD et USB) via libfat/devoptab, expose les préfixes
  `sd:/` et `usb:/` utilisés partout dans le code.
- Benchmark storage (`storage_test.c`) écrit/lit un fichier temporaire de
  1 Mo (`wiimedic_bench.tmp`) en blocs de 32 Ko, 3 itérations, sur `sd:` et
  `usb:` séparément — fichier supprimé après le test.

## Génération de rapport

- `run_report_generator()` (`report.c`) exécute séquentiellement les 6
  modules (system info, NAND health, IOS scan, storage, contrôleurs,
  réseau), en réutilisant les résultats déjà calculés si un module a déjà
  tourné depuis le menu principal (cache par flags `has_*_run()`).
- Rapport texte brut unique, sections `=== NOM ===`, écrit directement en
  streaming dans le fichier (`report_write` = `vfprintf`).
- **Pas de sortie JSON ni de format structuré** — uniquement `.txt` lisible
  humain. PinouDiag devra parser ce texte pour le normaliser (section 10 du
  brief) : format de sections assez régulier (`=== TITRE ===` puis lignes
  `Clé:            Valeur`), donc parsable par une regex/état simple, mais
  fragile aux futures évolutions de format côté WiiMedic upstream — à
  documenter comme dépendance externe versionnée (le `manifest.json` de
  l'addon doit épingler la version WiiMedic testée).

## Comportement à la fin

Voir `return_to_loader.md`. Résumé : jamais de retour à l'appelant ; sortie
= relance de title NAND ou reset vers System Menu.

## Lancement direct comme DOL / retour au loader

- **Lancement direct : oui**, confirmé (pas de garde HBC).
- **Retour au loader : non**, tel quel (voir `return_to_loader.md`).

## Autres observations utiles pour l'identité/fingerprint (section 7)

`system_info.c` expose, via libogc, sans dépendance à un service externe :
- `SYS_GetHollywoodRevision()` — révision matérielle du chipset Hollywood.
- `ES_GetDeviceID(&device_id)` — identifiant de device (32 bits) lié à
  l'appairage NAND/IOS, unique par console.
- `ES_GetBoot2Version()` — version de boot2.
- `CONF_GetRegion()` — région console (JP/US/EU/KR/CN).
- Détection Priiloader (présence + version) et compatibilité BootMii
  (boot2/IOS) via lecture OTP (`HW_REG_BASE = 0xCD000000`) — accès bas
  niveau au hardware Hollywood, fonctionne sans droits particuliers tant que
  `AHBPROT` est déverrouillé (ce qui est le cas sous homebrew standard).

Voir `identity.md` pour l'évaluation de ces candidats comme fingerprint.
