/*
 * PinouDiag Wii V1 - Core / Result (POC6, section 10 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Parse WiiMedic_Report.txt (texte libre, pas de format structure - voir
 * docs/wiimedic.md) vers le modele de resultat normalise pd_result_t.
 * Chaque regle d'extraction ci-dessous est ancree a une ligne precise du
 * code source WiiMedic (voir result.c) ; aucune n'est une supposition.
 */
#ifndef PD_RESULT_H
#define PD_RESULT_H

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Ajoute a 'out' un pd_result_t par regle reconnue dans report_text.
 * N'efface pas 'out' au prealable (permet d'accumuler plusieurs
 * sources - appeler pd_result_list_init() avant le premier appel).
 * Renvoie le nombre de resultats ajoutes. */
int pd_result_parse_wiimedic_report(const char *report_text, pd_result_list_t *out);

#ifdef __cplusplus
}
#endif

#endif /* PD_RESULT_H */
