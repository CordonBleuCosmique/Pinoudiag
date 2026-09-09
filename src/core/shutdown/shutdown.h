/*
 * PinouDiag Wii V1 - Core / Shutdown (POC7, section 15 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Sequence finale : ne doit etre appelee qu'apres confirmation d'ecriture
 * de report.json, report.txt et wiimedic.txt (verification faite par
 * l'appelant, voir core/report). Cette fonction ne verifie pas a nouveau
 * ces ecritures - elle suppose l'ordre respecte, conformement a la
 * section 15 : "Ne jamais supprimer PinouDiag.session avant la
 * sauvegarde correcte du rapport final."
 */
#ifndef PD_SHUTDOWN_H
#define PD_SHUTDOWN_H

#include "../pd_types.h"
#include "../session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Supprime le fichier de session, demonte/flush SD+USB, puis affiche
 * l'ecran SAFE_TO_REMOVE. A appeler une seule fois, en toute derniere
 * etape du workflow. */
pd_error_t pd_shutdown_finalize(pd_session_t *session);

#ifdef __cplusplus
}
#endif

#endif /* PD_SHUTDOWN_H */
