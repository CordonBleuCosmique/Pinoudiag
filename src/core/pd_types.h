/*
 * PinouDiag Wii V1 - types partages du Core
 *
 * [UNVERIFIED - non compile] Aucun toolchain devkitPPC n'est disponible
 * dans l'environnement qui a produit ce fichier (voir docs/build.md).
 * A compiler et valider avant tout usage sur materiel reel.
 */
#ifndef PD_TYPES_H
#define PD_TYPES_H

#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Section 13 du brief : codes d'erreur minimum */
typedef enum {
    PD_OK = 0,
    PD_ERR_UNSUPPORTED_WII,
    PD_ERR_USB_NOT_FOUND,
    PD_ERR_STORAGE_ERROR,
    PD_ERR_ADDON_NOT_FOUND,
    PD_ERR_ADDON_LOAD_ERROR,
    PD_ERR_ADDON_TIMEOUT,
    PD_ERR_ADDON_CRASH,
    PD_ERR_ADDON_RESULT_MISSING,
    PD_ERR_RESULT_PARSE_ERROR,
    PD_ERR_REPORT_WRITE_ERROR,
    PD_ERR_SESSION_ERROR,
} pd_error_t;

const char *pd_error_str(pd_error_t err);

/* Chemin fixe du binaire PinouDiag sur la carte SD - c'est exactement le
 * chemin que LetterBomb charge depuis la racine SD (voir
 * docs/letterbomb.md), et donc aussi le chemin de retour utilise par
 * tout addon/chainload qui doit rendre la main a PinouDiag (voir
 * docs/return_to_loader.md, Option A). PinouDiag/addons/workflows/config
 * restent dans un sous-dossier SD:/PinouDiag/ ; seul le binaire de boot
 * lui-meme vit a la racine, pour respecter le contrat LetterBomb sans
 * fichier de redirection supplementaire. */
#define PD_LOADER_BOOT_PATH "sd:/boot.dol"

/* Section 10 du brief : statuts de resultat normalise */
typedef enum {
    PD_STATUS_PASS = 0,
    PD_STATUS_WARN,
    PD_STATUS_FAIL,
    PD_STATUS_NOT_TESTED,
    PD_STATUS_ERROR,
} pd_status_t;

const char *pd_status_str(pd_status_t status);

#define PD_TEST_NAME_MAX   64
#define PD_TEST_DETAIL_MAX 256
#define PD_SOURCE_NAME_MAX 32

/* Un resultat de test normalise, independant du format d'origine de l'addon */
typedef struct {
    char         test[PD_TEST_NAME_MAX];
    pd_status_t  status;
    char         source[PD_SOURCE_NAME_MAX];   /* ex: "wiimedic" */
    char         detail[PD_TEST_DETAIL_MAX];   /* optionnel, ligne(s) associee(s) */
} pd_result_t;

#define PD_MAX_RESULTS 64

typedef struct {
    pd_result_t items[PD_MAX_RESULTS];
    int         count;
} pd_result_list_t;

void pd_result_list_init(pd_result_list_t *list);
bool pd_result_list_add(pd_result_list_t *list, const char *test,
                         pd_status_t status, const char *source,
                         const char *detail);

#ifdef __cplusplus
}
#endif

#endif /* PD_TYPES_H */
