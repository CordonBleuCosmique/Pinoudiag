# Wii Memory Map — zones pertinentes pour PinouDiag

[UNVERIFIED pour la carte mémoire complète — wiibrew.org/wiki/Memory_Map
inaccessible directement dans ce sandbox. Les adresses ci-dessous sont
[VERIFIED] par lecture directe du code source HBC (constantes utilisées en
production), ce qui est une preuve plus solide qu'une lecture de wiki mais
ne remplace pas la vérification de la carte complète.]

## MEM1 (24 Mo, `0x80000000`–`0x81800000` en adresses cached)

| Zone | Usage observé | Source |
|------|----------------|--------|
| `0x80000000`–`0x800000FF` | Registres SDK Nintendo (bus/CPU clock pokés par HBC avant un chainload : `0x800000F8`, `0x800000FC`) | `loader_reloc.c` [VERIFIED] |
| `0x80003140` | Version IOS courante (lue par `IOS_GetVersion()` dans le stub HBC via `DCInvalidateRange` + lecture directe) | `stub/stub.c` [VERIFIED] |
| `0x80001800`–`0x80003000` (zone signalée par le brief) | Zone basse utilisée par les **loaders** pour un "reload stub" qui survit à un `SYS_ResetSystem` non-complet | Corroboré indirectement : le "config" du stub HBC (`STUB_ADDR_MAGIC`/`STUB_ADDR_TITLE`) vit à `0x80002f00`/`0x80002f08`, à l'intérieur de cette plage [VERIFIED pour ces deux adresses précises ; UNVERIFIED pour l'étendue exacte de la zone réservée] |
| `0x80003400` | `LD_MIN_ADDR` — borne basse à partir de laquelle HBC accepte de charger des sections DOL/ELF | `loader_reloc.c`/`config.h` [VERIFIED] |
| `0x80003400`–`~0x81330000` | Zone d'exécution DOL/ELF (sections texte/data de l'app chargée) | `loader_reloc.c` (`LD_MIN_ADDR`/`LD_MAX_ADDR`) [VERIFIED] ; borne haute officielle communautaire souvent citée à `0x81200000` [UNVERIFIED, secondaire] |
| `~0x81330000`–`0x81800000` | Effacée par le stub d'exécution de HBC juste avant de sauter vers l'app chargée (`exec_stub`, boucle `stw r0,0(r9)` de `0x8133xxxx` à `0x8180xxxx`) | `loader_reloc.c` [VERIFIED] |

## MEM2 (64 Mo)

Utilisée par HBC pour placer le petit stub d'exécution avant le saut
(`SYS_GetArena2Hi() - sizeof(exec_stub)`, aligné 32 octets) — donc tout en
haut de l'arène MEM2 disponible au moment du chainload. [VERIFIED]

WiiMedic (`system_info.c`) rapporte `SYS_GetArena1Size()`/`SYS_GetArena2Size()`
comme "MEM1/MEM2 Arena Free" dans son écran Système — confirme que ces
arènes sont directement interrogeables via libogc à l'exécution. [VERIFIED]

## Implication pour le chainloader PinouDiag → WiiMedic

Un futur `dol_loader` PinouDiag doit :
1. Réserver/valider que les sections du DOL WiiMedic tiennent dans la plage
   `LD_MIN_ADDR`–`LD_MAX_ADDR` (ou équivalent) avant de copier quoi que ce
   soit — exactement la vérification `check_overlap` de `reloc_dol()`.
2. Ne pas placer ses propres structures de contrôle (état de session,
   pointeur de retour éventuel en cas d'option A du fork WiiMedic — voir
   `return_to_loader.md`) dans la zone qui sera effacée par le stub
   d'exécution (`~0x81330000`–`0x81800000`) ni dans la zone que WiiMedic
   écrasera lui-même (`LD_MIN_ADDR` et au-delà).
3. Si l'option A (fork WiiMedic avec chainload retour) est retenue, la zone
   basse `0x80001800`–`0x80003400` est la candidate naturelle pour y loger
   un petit marqueur/état minimal qui doit survivre au chainload — à
   confirmer par test réel qu'elle n'est effectivement jamais touchée ni par
   WiiMedic ni par le chainload lui-même.

## Statut

[REQUIRES HARDWARE TEST] pour toute utilisation de ces zones dans le code de
PinouDiag. [REQUIRES VERIFICATION] pour la carte mémoire complète via lecture
directe de WiiBrew si l'accès réseau est débloqué.
