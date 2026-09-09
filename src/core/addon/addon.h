/*
 * PinouDiag Wii V1 - Core / Addon system (POC5, section 8 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Le Core ne doit contenir aucune logique specifique a un addon donne
 * (pas de "if addon == wiimedic") - il fonctionne uniquement a partir du
 * manifest.json de chaque addon (section 8 du brief). WiiMedic est le
 * premier addon V1, mais rien ici ne le nomme en dur.
 */
#ifndef PD_ADDON_H
#define PD_ADDON_H

#include <stddef.h>

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PD_ADDON_ID_MAX       32
#define PD_ADDON_NAME_MAX     64
#define PD_ADDON_VERSION_MAX  32
#define PD_ADDON_PATH_MAX    256

typedef struct {
    char id[PD_ADDON_ID_MAX];
    char name[PD_ADDON_NAME_MAX];
    char version[PD_ADDON_VERSION_MAX];
    char platform[16];
    char executable[PD_ADDON_PATH_MAX];   /* relatif au dossier de l'addon, ex: "boot.dol" */
    char output[PD_ADDON_PATH_MAX];       /* nom de fichier de sortie attendu, ex: "WiiMedic_Report.txt" */
} pd_addon_t;

/* Lit et parse <addon_dir>/manifest.json. addon_dir est le chemin complet
 * du dossier de l'addon (ex: "sd:/PinouDiag/addons/WiiMedic"). */
pd_error_t pd_addon_load_manifest(const char *addon_dir, pd_addon_t *addon);

/* Parcourt addons_root (ex: "sd:/PinouDiag/addons") et renvoie dans
 * out_dir le chemin du premier sous-dossier dont manifest.json declare
 * "id" == addon_id. Ne suppose jamais que le nom du dossier correspond a
 * l'id (section 8 : "le Core doit fonctionner a partir du manifeste") -
 * resout au passage l'ecart entre l'id en minuscules des exemples du
 * brief (ex. "wiimedic") et le nom de dossier en usage
 * (ex. "WiiMedic") sans dependre d'une convention de casse. */
pd_error_t pd_addon_find_dir(const char *addons_root, const char *addon_id,
                              char *out_dir, size_t out_size);

/* Lance l'addon (chainload aller simple - voir docs/return_to_loader.md).
 * Ne revient qu'en cas d'echec de chargement ; en cas de succes, cet
 * appel ne revient jamais (l'execution continue dans l'addon puis, a sa
 * sortie, dans le second chainload qui recharge PinouDiag - voir
 * core/session pour la reprise au redemarrage). */
pd_error_t pd_addon_run(const pd_addon_t *addon, const char *addon_dir);

#ifdef __cplusplus
}
#endif

#endif /* PD_ADDON_H */
