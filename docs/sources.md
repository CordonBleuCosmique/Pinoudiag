# Sources — PinouDiag Wii V1

Format par ligne : Source | URL | Conclusion | Confiance | Statut de vérification.

Contrainte d'environnement notée une fois pour toutes : cette session de recherche
tourne dans un sandbox cloud dont le proxy réseau **bloque `wiibrew.org` et
`devkitpro.org` en accès direct** (WebFetch/curl → `EGRESS_BLOCKED`). `github.com`,
`raw.githubusercontent.com` et la recherche web (WebSearch) restent accessibles.
Le contenu WiiBrew ci-dessous provient donc soit de la synthèse WebSearch (qui
interroge un moteur tiers, pas notre proxy), soit de sources secondaires
(GBAtemp wiki, tockdom, gc-forever) recoupées avec le **code source réel** de
The Homebrew Channel et de WiiMedic (clonés directement depuis GitHub, lecture
primaire). Chaque fois que le code source contredisait ou précisait une source
secondaire, le code fait foi.

| # | Source | URL | Conclusion | Confiance | Statut |
|---|--------|-----|------------|-----------|--------|
| 1 | WiiBrew — LetterBomb (via WebSearch, wiibrew.org direct bloqué) | https://wiibrew.org/wiki/LetterBomb | Exploit du Wii Message Board sur System Menu 4.3 uniquement ; crash le canal Message Board pour exécuter du code qui charge `boot.elf` (fallback `boot.dol`) à la racine de la carte SD. Nécessite l'adresse MAC WiFi de la Wii pour générer la lettre. | Moyenne (source secondaire, non lue en direct) | [UNVERIFIED] — à confirmer par lecture directe de wiibrew.org si l'accès réseau le permet un jour, ou via une copie locale du wiki fournie par l'utilisateur |
| 2 | WiiBrew — Homebrew Channel / stub de reload (via WebSearch) | https://wiibrew.org/wiki/Homebrew_Channel | Le stub HBC vit en mémoire basse, autour de `0x80001800`, et survit à un soft-reset pour relancer HBC comme title NAND après un jeu. | Moyenne | [UNVERIFIED] pour l'adresse exacte côté wiki — mais **[VERIFIED]** indépendamment par lecture du code source `channel/channelapp/stub/` (voir source #4) qui confirme le principe (relance via title ID, pas un retour d'exécution) |
| 3 | DOL (File Format) — wiki.tockdom.com (via WebSearch, accès direct bloqué) | https://wiki.tockdom.com/wiki/DOL_(File_Format) | Format DOL : header 0x100 octets, 7 sections texte + 11 sections data, adresses/tailles par section, BSS, entry point. Plage d'adresses valides Wii/GC : `0x80004000`–`0x81200000`. | Haute (recoupée avec le code) | [VERIFIED] — recoupé avec `loader_reloc.c` de HBC (voir source #4), qui implémente un parseur DOL quasi identique et applique ses propres bornes `LD_MIN_ADDR=0x80003400` / `LD_MAX_ADDR` |
| 4 | **The Homebrew Channel — code source officiel** | https://github.com/dhtdht020/hbc (clone local `/home/user/research/hbc`, GPLv2) | Source primaire lue directement. Contient le vrai mécanisme de chargement de DOL/ELF (`loader_reloc.c`), le "reload stub" (`stub/stub.c`, `stub/crt0.S`) et la logique d'appel (`loader_exec`). Voir `dol_loading.md` et `return_to_loader.md` pour l'analyse détaillée. | Haute | [VERIFIED] — lecture directe du code, pas d'hypothèse |
| 5 | **WiiMedic — code source** | https://github.com/PowFPS1/WiiMedic (clone local `/home/user/powfps1/wiimedic`, GPLv2) | Source primaire lue directement (tout `source/*.c`, `Makefile`, `README.md`, `LICENSE`, `meta.xml`). Voir `wiimedic.md`. | Haute | [VERIFIED] |
| 6 | devkitPro / devkitPPC (via WebSearch, devkitpro.org direct bloqué) | https://devkitpro.org/wiki/Getting_Started/devkitPPC | Toolchain GCC PowerPC pour Wii/GameCube ; installation via pacman devkitPro. | Haute (usage confirmé par le Makefile de WiiMedic et de HBC, qui exigent tous deux `DEVKITPPC`) | [VERIFIED] pour l'usage attendu ; [UNVERIFIED] pour la procédure d'installation exacte, non testée dans ce sandbox (voir `build.md`) |
| 7 | libogc | https://github.com/devkitPro/libogc | Bibliothèque bas niveau Wii/GC (vidéo, pad, ES/IOS, fat, réseau...). Utilisée par WiiMedic et HBC. | Haute (usage confirmé par les deux bases de code) | [UNVERIFIED] — dépôt non cloné directement dans cette session (non nécessaire : les usages observés dans WiiMedic/HBC suffisent pour l'instant) |
| 8 | GBAtemp wiki — Letterbomb (via WebSearch) | https://wiki.gbatemp.net/wiki/Letterbomb | Recoupe la description de LetterBomb (source #1). | Moyenne | [UNVERIFIED] (secondaire) |
| 9 | Recherche "WinterMute/dolloader" | (introuvable) | Un résultat de recherche mentionnait ce dépôt comme loader DOL avec retour au loader. **Le dépôt n'existe pas** à ce chemin (`git clone` échoue avec une demande d'authentification typique d'un 404 sur dépôt public inexistant). | — | [UNVERIFIED] / **infirmé** — ne pas réutiliser cette référence |

## Conclusion de recherche la plus critique (section 4 du brief)

Voir `return_to_loader.md`. Résumé : **WiiMedic, tel quel, ne peut pas rendre la
main à PinouDiag par un simple retour de fonction.** Son code (`source/main.c`)
termine toujours par `WII_LaunchTitle()` (relance d'un title NAND) ou
`SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0)` (reset vers le System Menu) — jamais
par un `return` normal atteignant l'appelant. Par ailleurs, le chargeur DOL de
référence (HBC `loader_exec()`) est lui-même conçu comme un **aller simple**
(il coupe les services IOS, ferme les exceptions, efface une partie de la
mémoire haute, puis saute — sans retour possible) et non comme un appel de
sous-routine. Ces deux faits combinés remettent en cause l'hypothèse implicite
du brief ("WiiMedic termine → PinouDiag reprend" par simple chainload/retour).
Voir `return_to_loader.md` pour les options.
