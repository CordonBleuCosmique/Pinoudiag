# Chargement de DOL/ELF — format et mécanique

Source primaire : `channel/channelapp/source/loader_reloc.c` (HBC, GPLv2,
[VERIFIED] par lecture directe). Recoupé avec la description du format DOL de
wiki.tockdom.com (via WebSearch, [UNVERIFIED] en direct).

## Format DOL (structure `dolheader`, 0x100 octets)

```c
typedef struct _dolheader {
    u32 text_pos[7];     // offset dans le fichier de chaque section texte
    u32 data_pos[11];    // offset dans le fichier de chaque section data
    u32 text_start[7];   // adresse mémoire cible de chaque section texte
    u32 data_start[11];  // adresse mémoire cible de chaque section data
    u32 text_size[7];
    u32 data_size[11];
    u32 bss_start;
    u32 bss_size;
    u32 entry_point;
} dolheader;
```

7 sections de code exécutable max, 11 sections de données max. Valeurs
big-endian (PowerPC). [VERIFIED]

## Format ELF (alternative acceptée par HBC)

`loader_reloc()` détecte d'abord si le binaire est un ELF32 big-endian
PowerPC exécutable (`is_valid_elf`), sinon il tente un parse DOL. Pour l'ELF,
chaque `PT_LOAD` phdr est copié à `p_paddr` (masqué sur 0x3FFFFFFF puis
OR 0x80000000 pour retomber dans l'espace mémoire physique Wii). Le point
d'entrée est `e_entry` masqué de la même façon. [VERIFIED]

WiiMedic compile en `.dol` (voir son Makefile : `elf2dol boot.elf boot.dol`),
donc PinouDiag n'a besoin de supporter que le format DOL pour l'addon
WiiMedic — mais supporter aussi l'ELF (comme HBC) coûte peu et généralise
proprement le loader pour de futurs addons.

## Contraintes d'adresses (constantes HBC, `config.h`)

```c
#define LD_MIN_ADDR 0x80003400
#define LD_MAX_ADDR (BASE_ADDR - 1 - ARGS_MAX_LEN)   // proche du haut de MEM1
#define LD_MAX_SIZE (LD_MAX_ADDR - LD_MIN_ADDR)
#define LD_ARGS_ADDR (LD_MAX_ADDR + 1)
```

`reloc_dol()`/`reloc_elf()` vérifient (`check_overlap`) que chaque section
chargée reste dans `[LD_MIN_ADDR, LD_MAX_ADDR]` avant de faire le `memmove`.
Cette plage correspond à ce que les wikis communautaires (wiki.tockdom.com,
gc-forever) décrivent comme la zone valide pour un exécutable DOL sur
Wii/GameCube (`0x80004000`–`0x81200000` selon la source secondaire — HBC est
légèrement plus restrictif côté borne basse, probablement pour laisser de la
marge au-dessus de sa propre zone basse réservée). [VERIFIED pour les bornes
HBC ; UNVERIFIED pour la plage exacte "officielle" faute d'accès direct à
WiiBrew]

## Passage d'arguments (`argv`)

```c
static void set_argv (entry_point *ep, const char *args, u16 arg_len) {
    u32 *p = (u32 *) *ep;
    if (p[1] != ARGV_MAGIC) return;   // l'app ne supporte pas argv, on n'écrit rien
    struct __argv *argv = (struct __argv *) &p[2];
    ...
    argv->argvMagic = ARGV_MAGIC;
    argv->commandLine = cmdline;      // copié à LD_ARGS_ADDR
    argv->length = arg_len;
}
```

Convention libogc standard (`p[1] == ARGV_MAGIC` juste après l'entry point)
pour détecter si le binaire cible sait lire des arguments de ligne de
commande. WiiMedic (`source/main.c`, `int main(int argc, char **argv)`) suit
cette convention par défaut du starter devkitPPC/libogc — donc PinouDiag
**peut** en théorie lui passer des arguments (ex. un chemin de sortie de
rapport), mais WiiMedic ne les lit/n'exploite actuellement aucun `argv[]`
dans son code (chemins `sd:/WiiMedic_Report.txt` / `usb:/WiiMedic_Report.txt`
en dur). [VERIFIED : main.c ignore argc/argv]

## Séquence de chargement + saut (`loader_reloc` + `loader_exec`)

Voir `return_to_loader.md` pour l'analyse complète (c'est un aller simple,
pas un appel de sous-routine). Résumé technique :

1. `patch_crt0(ep)` — patch un octet du crt0 de l'app chargée si besoin
   (`p[0x20] == 0x41` → `0x40`), suivi d'un `DCFlushRange`.
2. `set_argv()` si l'app supporte argv.
3. `loader_exec(ep)` : arrêt propre des services (`SYS_ResetSystem(SYS_SHUTDOWN,...)`),
   fermeture des exceptions, poke des registres horloge SDK, construction
   d'un stub exécutable en haute mémoire (MEM2) qui efface une plage mémoire
   puis fait `bctr` vers `ep`.

Toute implémentation PinouDiag d'un loader DOL pour WiiMedic devrait
réutiliser cette séquence quasi telle quelle (GPLv2, attribution requise —
voir `licensing.md`) plutôt que la réinventer, conformément à la règle du
brief "ne pas réimplémenter si une solution open source fiable existe".
[VERIFIED que ce chemin fonctionne dans HBC en production ; REQUIRES HARDWARE
TEST pour toute réutilisation dans PinouDiag]
