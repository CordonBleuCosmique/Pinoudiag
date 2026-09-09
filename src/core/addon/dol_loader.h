/*
 * PinouDiag Wii V1 - Core / Addon / DOL loader (POC4)
 * [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * Chainload DOL en memoire, adapte de la sequence loader_reloc.c /
 * loader_exec() de The Homebrew Channel (dhtdht020/hbc, GPLv2 or later,
 * copyright 2008 dhewg/marcan #wiidev efnet) - voir docs/dol_loading.md
 * et docs/return_to_loader.md pour l'analyse complete qui justifie ce
 * choix (reutilisation plutot que reimplementation, brief section 0/19).
 *
 * Point essentiel documente dans docs/return_to_loader.md : ce mecanisme
 * est un ALLER SIMPLE (comme un execve), pas un appel de sous-routine.
 * pd_dol_exec() ne revient jamais a l'appelant sur materiel reel - les
 * services sont arretes et une partie de la memoire est effacee avant
 * le saut. Pour un "retour au loader", le binaire cible doit lui-meme
 * rappeler pd_dol_exec() vers le chemin du loader d'origine (c'est
 * exactement ce que fait le fork WiiMedic - voir addons/WiiMedic/ et
 * tools/test_dol/).
 *
 * Ce module est distribue sous GPLv2 or later (meme licence que le code
 * dont il derive) - voir docs/licensing.md.
 */
#ifndef PD_DOL_LOADER_H
#define PD_DOL_LOADER_H

#include <gctypes.h>

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*pd_entry_point_t)(void);

/* Charge un fichier .dol depuis le stockage monte (chemin type "sd:/x.dol"
 * ou "usb:/x.dol") en memoire, aux adresses demandees par son header, et
 * renvoie son point d'entree dans *entry. Ne modifie aucune ressource
 * systeme (pas d'arret de service) - c'est pd_dol_exec() qui fait le saut
 * definitif. Permet de valider le chargement (fichier trouve, DOL valide,
 * sections dans les bornes) avant d'engager un aller sans retour. */
pd_error_t pd_dol_load(const char *path, pd_entry_point_t *entry);

/* Point de non-retour : arrete les services (reseau, IOS courant),
 * ferme les gestionnaires d'exception, efface le haut de la memoire,
 * puis saute vers entry. N'importe quel code apres cet appel, sur
 * materiel reel, n'est jamais execute. [REQUIRES HARDWARE TEST] */
void pd_dol_exec(pd_entry_point_t entry) __attribute__((noreturn));

/* Enchaine pd_dol_load() + pd_dol_exec(). Ne revient qu'en cas d'erreur
 * de chargement (fichier absent, DOL invalide, section hors bornes) -
 * si elle revient, aucun saut n'a eu lieu. */
pd_error_t pd_dol_chainload(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* PD_DOL_LOADER_H */
