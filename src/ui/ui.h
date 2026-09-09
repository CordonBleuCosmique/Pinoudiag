/*
 * PinouDiag Wii V1 - interface texte minimale
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Priorites (section 12 du brief) : lisibilite, stabilite, progression,
 * erreurs comprehensibles. Pas de dependance graphique au-dela de la
 * console libogc standard.
 */
#ifndef PD_UI_H
#define PD_UI_H

#include <gctypes.h>

#include "../core/pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void pd_ui_init(void);
void pd_ui_clear(void);
void pd_ui_banner(const char *console_model, const char *system_version);

/* Ligne de checklist d'etape : marque DONE ('v'), CURRENT ('>') ou PENDING ('o') */
typedef enum { PD_STEP_PENDING = 0, PD_STEP_CURRENT, PD_STEP_DONE, PD_STEP_ERROR } pd_step_state_t;

void pd_ui_progress_begin(void);
void pd_ui_progress_step(const char *label, pd_step_state_t state);
void pd_ui_progress_bar(int percent);
void pd_ui_progress_end(void);

/* Message pour un addon qui ne fournit pas de progression fine (section 12) */
void pd_ui_busy(const char *addon_name, const char *message);

void pd_ui_error(pd_error_t err, const char *context);
void pd_ui_info(const char *msg);

void pd_ui_safe_to_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* PD_UI_H */
