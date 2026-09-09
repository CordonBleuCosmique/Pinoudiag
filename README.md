# PinouDiag Wii V1

Orchestrateur autonome de diagnostic pour Nintendo Wii. PinouDiag ne
réimplémente pas les diagnostics existants : il détecte la console, prépare
le stockage, charge un workflow, lance des addons de diagnostic existants
(premier et unique addon V1 : [WiiMedic](https://github.com/PowFPS1/WiiMedic)),
normalise leurs résultats et génère un rapport — sans installer le Homebrew
Channel, sans modifier la console de façon permanente, sans fonction de
réparation.

Workflow : `LetterBomb → PinouDiag → identification → stockage → WiiMedic →
analyse → rapport → SAFE_TO_REMOVE`.

## Statut

Développement guidé par la recherche (voir `docs/`) avant l'implémentation,
conformément au brief du projet. Point d'avancement :

- ✅ Recherche technique (sources, licences, architecture mémoire, format
  DOL, mécanisme de retour au loader) — voir `docs/`.
- ✅ Code source complet pour la V1 (bootstrap, stockage, session, workflow,
  addon, résultats, rapport, fingerprint, chainloader) — voir `src/`.
- ✅ Fork patché de WiiMedic pour le retour vers PinouDiag — voir
  `third_party/WiiMedic-PinouDiag/`.
- ⚠️ **Rien de tout cela n'a été compilé ni testé sur matériel réel.**
  Aucun toolchain devkitPPC n'est disponible dans l'environnement qui a
  produit ce code (voir `docs/build.md` pour le détail). Tout le code est
  marqué `[UNVERIFIED]` / `[REQUIRES HARDWARE TEST]` en conséquence.

## Structure du dépôt

```
docs/                    Recherche (sources, licences, architecture)
src/
├── main/                Point d'entree (orchestration)
├── core/
│   ├── bootstrap/        Init video/pads
│   ├── storage/          SD (lecture seule) / USB (persistant)
│   ├── detection/        Compatibilite Wii V1
│   ├── identity/         ConsoleFingerprint
│   ├── addon/             Systeme d'addon generique + chainloader DOL
│   ├── workflow/         Moteur de workflow (JSON)
│   ├── result/            Normalisation des resultats WiiMedic
│   ├── report/            report.json / report.txt / identity.json
│   ├── session/           PinouDiag.session
│   └── shutdown/          Sequence finale / SAFE_TO_REMOVE
├── ui/                    Interface texte
└── platform/wii/          (reserve)
addons/WiiMedic/           manifest.json (+ boot.dol une fois compile)
workflows/wii_v1.json       Workflow V1
third_party/WiiMedic-PinouDiag/  Fork GPLv2 de WiiMedic (voir CHANGES.md)
tools/test_dol/             Binaire de validation du chainload (POC4)
```

## Build

Voir `docs/build.md`. Résumé : nécessite devkitPPC + libogc (non installés
dans cet environnement de développement).

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC

make && make sd-image                              # PinouDiag -> dist/sd/
cd tools/test_dol && make                           # validation POC4
cd third_party/WiiMedic-PinouDiag && make install   # addon WiiMedic -> addons/WiiMedic/
```

## Licence

Code PinouDiag propre : voir la licence du dépôt. Le fork WiiMedic
(`third_party/WiiMedic-PinouDiag/`) et le chainloader DOL
(`src/core/addon/dol_loader.c`, adapté de The Homebrew Channel) sont
distribués sous GPLv2 or later — voir `docs/licensing.md` pour le détail
des obligations et attributions.
