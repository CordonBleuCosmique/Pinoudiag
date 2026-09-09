/*
 * PinouDiag Wii V1 - Core / Detection
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * V1 = Wii uniquement (section 0 du brief). Le point d'entree PinouDiag
 * n'est atteint qu'apres que LetterBomb ait deja execute boot.elf/boot.dol
 * depuis la racine SD (voir docs/letterbomb.md) - un contexte qui n'existe
 * que sur Wii avec System Menu 4.3. Ce module reste une securite
 * defensive, pas la ligne de defense principale.
 */
#ifndef PD_DETECTION_H
#define PD_DETECTION_H

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char model[16];    /* ex: "Wii RVL-001" - best effort, voir .c */
    char system[16];   /* ex: "4.3E" - best effort */
} pd_console_info_t;

/* Renvoie PD_OK si l'environnement d'execution est compatible V1,
 * PD_ERR_UNSUPPORTED_WII sinon. Remplit info dans tous les cas ou possible. */
pd_error_t pd_detection_check(pd_console_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* PD_DETECTION_H */
