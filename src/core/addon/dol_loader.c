/*
 * [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * Adapte de channel/channelapp/source/loader_reloc.c, The Homebrew
 * Channel (https://github.com/dhtdht020/hbc), GPLv2 or later,
 * Copyright (C) 2008 dhewg, #wiidev efnet ; Copyright (C) 2008 marcan,
 * #wiidev efnet. Voir docs/licensing.md pour le detail des obligations
 * de redistribution appliquees a ce fichier.
 *
 * Modifications par rapport a la source HBC :
 *  - support DOL uniquement (pas d'ELF - inutile pour V1, voir
 *    docs/dol_loading.md : WiiMedic et test.dol compilent en .dol) ;
 *  - lecture du fichier depuis le stockage monte (fopen/fread) plutot
 *    que reception reseau/USB Gecko (hors perimetre PinouDiag) ;
 *  - pas de gestion argv (les addons PinouDiag ne lisent pas d'argv,
 *    voir docs/wiimedic.md) ;
 *  - API adaptee aux types/erreurs pd_* de ce projet.
 */
#include <malloc.h>
#include <ogcsys.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dol_loader.h"

extern void __exception_closeall(void);

/* Bornes verifiees par lecture du code HBC (config.h : LD_MIN_ADDR) pour
 * la borne basse. Borne haute reprise de sources secondaires
 * (wiki.tockdom.com, gc-forever) recoupees mais NON confirmees via
 * WiiBrew direct (inaccessible pendant la recherche) - voir
 * docs/dol_loading.md. Reste tres proche de la borne haute effective
 * MEM1 (0x81800000) moins la marge que HBC reserve pour son propre
 * exec_stub. */
#define PD_DOL_MIN_ADDR 0x80003400u
#define PD_DOL_MAX_ADDR 0x81200000u

typedef struct {
    u32 text_pos[7];
    u32 data_pos[11];
    u32 text_start[7];
    u32 data_start[11];
    u32 text_size[7];
    u32 data_size[11];
    u32 bss_start;
    u32 bss_size;
    u32 entry_point;
} pd_dolheader_t;

static u8 *read_file(const char *path, u32 *size_out) {
    FILE *fp;
    long size;
    u8 *buf;

    fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) {
        fclose(fp);
        return NULL;
    }

    buf = (u8 *) malloc((size_t) size);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    if (fread(buf, 1, (size_t) size, fp) != (size_t) size) {
        fclose(fp);
        free(buf);
        return NULL;
    }

    fclose(fp);
    *size_out = (u32) size;
    return buf;
}

static bool reloc_dol(pd_entry_point_t *ep, const u8 *addr, u32 size) {
    const pd_dolheader_t *dol = (const pd_dolheader_t *) addr;
    u32 i;

    for (i = 0; i < 7; i++) {
        if (!dol->text_size[i])
            continue;

        if (dol->text_pos[i] + dol->text_size[i] > size)
            return false;

        if (dol->text_start[i] < PD_DOL_MIN_ADDR ||
            dol->text_start[i] + dol->text_size[i] > PD_DOL_MAX_ADDR)
            return false;

        memmove((void *) dol->text_start[i], addr + dol->text_pos[i], dol->text_size[i]);
        DCFlushRange((void *) dol->text_start[i], dol->text_size[i]);
        ICInvalidateRange((void *) dol->text_start[i], dol->text_size[i]);
    }

    for (i = 0; i < 11; i++) {
        if (!dol->data_size[i])
            continue;

        if (dol->data_pos[i] + dol->data_size[i] > size)
            return false;

        if (dol->data_start[i] < PD_DOL_MIN_ADDR ||
            dol->data_start[i] + dol->data_size[i] > PD_DOL_MAX_ADDR)
            return false;

        memmove((void *) dol->data_start[i], addr + dol->data_pos[i], dol->data_size[i]);
        DCFlushRange((void *) dol->data_start[i], dol->data_size[i]);
    }

    *ep = (pd_entry_point_t) dol->entry_point;

    return true;
}

pd_error_t pd_dol_load(const char *path, pd_entry_point_t *entry) {
    u8 *buf;
    u32 size = 0;
    bool ok;

    buf = read_file(path, &size);
    if (!buf || size < sizeof(pd_dolheader_t))
        return PD_ERR_ADDON_LOAD_ERROR;

    ok = reloc_dol(entry, buf, size);

    /* Le buffer temporaire n'est plus necessaire une fois les sections
     * copiees a leurs adresses cibles (comportement identique a
     * loader_reloc.c, meme mise en garde sur un chevauchement theorique
     * buffer/cible - voir commentaire en tete de fichier). */
    free(buf);

    return ok ? PD_OK : PD_ERR_ADDON_LOAD_ERROR;
}

/* Stub d'execution : identique dans son principe a exec_stub de
 * loader_reloc.c (HBC) - un tout petit code machine copie en haute
 * memoire qui efface une plage puis saute vers l'entry point (bctr,
 * jamais de retour). Instructions PowerPC (big-endian) :
 *   mtctr r3 ; lis r9,HI(CLEAR_START) ; lis r10,HI(CLEAR_END) ;
 *   1: stw r0,0(r9) ; addi r9,r9,4 ; cmpd r9,r10 ; blt 1b ; bctr
 * reprises telles quelles de la source HBC (memes constantes
 * d'effacement haute-memoire, voir docs/memory_map.md). */
static const u32 s_exec_stub[] = {
    0x7c6903a6, /* mtctr r3 */
    0x3d208133, /* lis r9, 0x8133 */
    0x3d408180, /* lis r10, 0x8180 */
    0x90090000, /* 1: stw r0, 0(r9) */
    0x39290004, /* addi r9, r9, 4 */
    0x7c295000, /* cmpd r9, r10 */
    0x4180fff4, /* blt 1b */
    0x4e800420  /* bctr */
};

void pd_dol_exec(pd_entry_point_t entry) {
    void *target;
    void (*f_exec_stub)(int);

    /* Arret des services (reseau, threads, IOS courant) - pas un reset
     * materiel complet. Point de non-retour a partir d'ici. */
    SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

    __exception_closeall();

    /* Registres horloge bus/CPU attendus par les DOL construits avec le
     * SDK Nintendo - repris tels quels de loader_reloc.c. */
    *(vu32 *) 0x800000F8 = 0x0E7BE2C0;
    *(vu32 *) 0x800000FC = 0x2B73A840;

    target = (void *) (((u32) SYS_GetArena2Hi() - sizeof(s_exec_stub)) & ~31u);
    f_exec_stub = (void (*)(int)) target;

    memcpy(target, s_exec_stub, sizeof(s_exec_stub));
    DCFlushRange(target, sizeof(s_exec_stub));
    ICInvalidateRange(target, sizeof(s_exec_stub));

    f_exec_stub((u32) entry);

    /* Jamais atteint sur materiel reel si le stub s'execute correctement. */
    for (;;) { }
}

pd_error_t pd_dol_chainload(const char *path) {
    pd_entry_point_t entry = NULL;
    pd_error_t err = pd_dol_load(path, &entry);

    if (err != PD_OK)
        return err;

    pd_dol_exec(entry); /* ne revient pas */

    return PD_OK; /* jamais atteint */
}
