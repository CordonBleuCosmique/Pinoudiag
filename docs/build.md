# Build — toolchain et statut dans cet environnement

## Constat pour cet environnement de développement (sandbox distant)

[VERIFIED — testé directement dans cette session]

- `devkitPPC`/`devkitPro` **non installés** (`$DEVKITPRO`/`$DEVKITPPC` vides,
  `/opt/devkitpro` absent).
- `devkitpro.org` est **bloqué par le proxy réseau** de ce sandbox
  (`EGRESS_BLOCKED` en WebFetch, `CONNECT tunnel failed, response 403` en
  curl direct) — impossible de suivre la procédure d'installation standard
  (pacman devkitPro) telle quelle depuis cet environnement.
- `github.com` et `raw.githubusercontent.com` restent accessibles (clone
  git anonyme confirmé : `PowFPS1/WiiMedic` et `dhtdht020/hbc` clonés avec
  succès). `api.github.com` en revanche renvoie 403 sur ce sandbox.
- Aucune Wii physique ni émulateur (Dolphin) disponible ici.

**Conséquence directe** : aucun code PowerPC écrit dans le cadre de ce
projet ne peut être compilé ni testé dans cette session. Tout code produit
ici sera marqué [UNVERIFIED — non compilé] jusqu'à validation dans un
environnement disposant du toolchain (poste de l'utilisateur, CI dédiée,
etc.).

## Procédure de build attendue (d'après les Makefiles WiiMedic et HBC,
[VERIFIED] par lecture directe, non testée dans ce sandbox)

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=/opt/devkitpro/devkitPPC
make            # -> boot.elf, boot.dol
make dist       # -> zip prêt à distribuer (pattern WiiMedic)
```

Dépendances : devkitPPC (GCC PowerPC), libogc ≥3.0.0, libfat, wiiuse, bte.
Le Makefile HBC ajoute des dépendances supplémentaires (zlib, libpng, mxml,
freetype, + outils hôte : pycryptodomex, libpng-dev, gettext, sox) — non
nécessaires pour PinouDiag lui-même (PinouDiag n'a pas besoin de
fonctionnalités réseau/thème/banner comme HBC), sauf si le module
`dol_loader` adapté de HBC entraîne une dépendance transitoire à auditer
précisément au moment de l'intégration du code.

## Recommandation

Deux options pour la suite :

1. **L'utilisateur fournit un environnement avec devkitPPC** (poste local ou
   CI GitHub Actions avec une image devkitPro) pour la compilation et les
   tests — cette session produit le code source, la structure SD/USB, les
   scripts de build, mais ne peut pas prouver qu'ils compilent.
2. **Tenter une installation manuelle du toolchain** dans ce sandbox par
   d'autres voies que `devkitpro.org` (ex. si un miroir GitHub Releases des
   binaires devkitPPC est accessible via `github.com`, qui lui fonctionne
   ici) — à explorer si souhaité, mais pas garanti de fonctionner selon la
   politique réseau du sandbox.

Aucune de ces deux options n'a été tranchée à ce stade — c'est un point à
discuter avant de lancer l'implémentation (POC 1 et suivants), en plus de la
question d'architecture posée dans `return_to_loader.md`.
