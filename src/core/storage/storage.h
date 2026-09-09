/*
 * PinouDiag Wii V1 - Core / Storage (POC2)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Regles du brief (section 5) :
 *   - SD  = lecture seule pendant toute l'execution de PinouDiag.
 *   - USB = stockage persistant des resultats (Results/, session).
 *   - Ne jamais ecraser d'anciens diagnostics.
 *
 * Note (voir docs/usb_sd.md) : WiiMedic (l'addon), lui, peut ecrire sur
 * SD si elle est presente et accessible en ecriture (chemin en dur dans
 * son code, hors du controle de PinouDiag). Ce module ne garantit donc
 * la regle "SD = lecture seule" que pour le code de PinouDiag lui-meme ;
 * voir core/result pour le nettoyage post-addon.
 */
#ifndef PD_STORAGE_H
#define PD_STORAGE_H

#include <gctypes.h>

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool sd_present;
    bool usb_present;
} pd_storage_state_t;

/* Monte SD et USB independamment (chacun peut echouer sans bloquer l'autre).
 * Renvoie PD_OK si au moins un device est monte, PD_ERR_STORAGE_ERROR sinon. */
pd_error_t pd_storage_init(pd_storage_state_t *state);

/* Re-detecte la presence sans remonter (pour rafraichir l'etat pendant le run) */
void pd_storage_refresh(pd_storage_state_t *state);

/* Flush + demontage propre avant SAFE_TO_REMOVE (section 15) */
pd_error_t pd_storage_shutdown(void);

/* Prefixes device (constants exposees pour les autres modules) */
#define PD_SD_PREFIX  "sd:"
#define PD_USB_PREFIX "usb:"

#ifdef __cplusplus
}
#endif

#endif /* PD_STORAGE_H */
