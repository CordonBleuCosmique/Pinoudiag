/*
 * PinouDiag Wii V1 - Core / Workflow engine (POC7, section 9 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Ce module ne fait que charger/representer un workflow depuis un
 * fichier JSON (ex: workflows/wii_v1.json) - il ne connait aucun step id
 * particulier ("identify", "wiimedic", ...). L'execution concrete des
 * steps "internal" est fournie par l'appelant (src/main/main.c), qui est
 * le point naturel d'orchestration de haut niveau ; les steps "addon"
 * sont, elles, generiques (pilotees par manifest.json via core/addon,
 * section 8) et ne necessitent aucune connaissance specifique ici.
 */
#ifndef PD_WORKFLOW_H
#define PD_WORKFLOW_H

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PD_WORKFLOW_MAX_STEPS 16
#define PD_WORKFLOW_ID_MAX    32
#define PD_WORKFLOW_NAME_MAX  64

typedef enum {
    PD_STEP_TYPE_INTERNAL = 0,
    PD_STEP_TYPE_ADDON,
} pd_step_type_t;

typedef struct {
    char           id[PD_WORKFLOW_ID_MAX];
    pd_step_type_t type;
    char           addon[PD_WORKFLOW_ID_MAX]; /* uniquement si type == ADDON */
} pd_workflow_step_t;

typedef struct {
    char               id[PD_WORKFLOW_ID_MAX];
    char               name[PD_WORKFLOW_NAME_MAX];
    pd_workflow_step_t steps[PD_WORKFLOW_MAX_STEPS];
    int                step_count;
} pd_workflow_t;

/* Charge et valide un workflow depuis un fichier JSON. Une etape sans
 * "id" reconnu ou de type ni "internal" ni "addon" fait echouer le
 * chargement entier (section 9 : "valide les etapes"). */
pd_error_t pd_workflow_load(const char *path, pd_workflow_t *wf);

/* Renvoie l'index de l'etape step_id, ou -1 si absente. */
int pd_workflow_find_index(const pd_workflow_t *wf, const char *step_id);

#ifdef __cplusplus
}
#endif

#endif /* PD_WORKFLOW_H */
