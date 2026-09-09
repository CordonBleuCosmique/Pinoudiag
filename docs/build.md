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

## Tentative d'installation du toolchain dans ce sandbox — [VERIFIED, échec documenté]

Sept canaux de distribution testés directement depuis cette session, tous
bloqués par la politique réseau du sandbox (proxy à liste blanche stricte,
confirmé via `/__agentproxy/status` : seuls npm, jsr.io, PyPI, crates.io et
le proxy Go modules sont en accès direct hors proxy ; tout le reste passe
par un proxy qui n'autorise qu'un sous-ensemble restreint de domaines) :

| Canal tenté | Résultat |
|---|---|
| `devkitpro.org` (procédure pacman officielle) | `403` — bloqué |
| `pacman.devkitpro.org` (dépôt pacman direct) | `403` — bloqué |
| `sourceforge.net` (portlibs, cité par le README de HBC) | `403` — bloqué |
| Image Docker officielle `devkitpro/devkitppc` (Docker Hub) | Manifest accessible via `registry-1.docker.io`, mais les **blobs** (656 Mo) sont servis par `production.cloudfront.docker.com`, bloqué (`403`) — daemon Docker démarré avec succès dans ce sandbox (`dockerd` root, cgroup v1), mais le pull échoue à la couche CDN |
| `ghcr.io/devkitpro/devkitppc` | `401 denied` — image absente à ce chemin sur ce registre |
| `quay.io` | `403` — bloqué |
| GNU FTP/mirrors pour reconstruire un cross-toolchain PowerPC-eabi/newlib depuis les sources (gcc, binutils, newlib) | `ftp.gnu.org`, `gcc.gnu.org`, `sourceware.org`, `ftpmirror.gnu.org`, `mirrors.kernel.org` — tous `403` |

**Conclusion : l'acquisition du toolchain devkitPPC est infaisable dans ce
sandbox avec la politique réseau actuelle**, quelle que soit la méthode
(installeur officiel, image Docker pré-construite, ou reconstruction depuis
les sources). Ce n'est pas un problème de méthode mais de politique réseau
organisationnelle (liste blanche stricte côté proxy).

## Recommandation

1. **L'utilisateur fournit un environnement avec devkitPPC** (poste local ou
   CI GitHub Actions avec l'image `devkitpro/devkitppc`, qui elle a accès
   sans restriction à Docker Hub) pour la compilation et les tests — cette
   session continue à produire le code source, la structure SD/USB et les
   scripts de build, marqués **[UNVERIFIED — non compilé]** jusqu'à
   validation dans un tel environnement.
2. Si l'accès sandbox est indispensable, la seule voie possible serait que
   l'administrateur de l'organisation ajoute `devkitpro.org` (ou
   `production.cloudfront.docker.com` pour Docker Hub) à la liste blanche du
   proxy réseau — hors de portée de cette session.

Décision : on continue l'implémentation (POC 1 et suivants) sans compilation
possible dans ce sandbox ; tout le code sera explicitement marqué
[UNVERIFIED — non compilé] jusqu'à build/test côté utilisateur.
