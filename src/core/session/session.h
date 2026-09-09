/*
 * PinouDiag Wii V1 - Core / Session (POC3, section 6 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Fichier unique de memoire d'execution : USB:/PinouDiag.session
 *
 * Regles (section 6 du brief, avec une precision necessaire - voir
 * paragraphe suivant) :
 *  - Sauvegarde a chaque mutation (etape, resultat, erreur) pour
 *    permettre une reprise fidele en cas de coupure.
 *  - Ne jamais supprimer le fichier de session avant que report.json/
 *    report.txt/wiimedic.txt soient confirmes ecrits (pd_session_end
 *    est le seul point qui supprime, et c'est a l'appelant de garantir
 *    l'ordre - voir core/shutdown).
 *
 * Precision par rapport au texte litteral du brief ("un fichier existant
 * est une session interrompue, on le supprime toujours") : ce
 * comportement est INCOMPATIBLE avec l'Option A retenue dans
 * docs/return_to_loader.md, qui redemarre completement PinouDiag apres
 * chaque addon (chainload aller simple, pas de call/return possible sur
 * Wii - voir ce document). Sans reconnaitre ce cas, PinouDiag effacerait
 * sa propre session juste avant de reprendre apres WiiMedic et
 * relancerait tout depuis le debut a l'infini.
 *
 * Regle appliquee ici (validee implicitement par l'utilisateur en
 * choisissant l'Option A, qui decrit explicitement ce mecanisme) :
 *   - state == RUNNING  -> reprise (le fichier est charge tel quel, pas
 *     supprime). Les etapes internes etant idempotentes, reprendre une
 *     session RUNNING est sans risque meme apres une vraie coupure de
 *     courant (pas seulement apres un chainload volontaire).
 *   - state == DONE / ERROR / fichier illisible -> traite comme
 *     interrompu au sens du brief : journalise, supprime, nouvelle
 *     session creee.
 */
#ifndef PD_SESSION_H
#define PD_SESSION_H

#include <gctypes.h>

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PD_SESSION_PATH   "usb:/PinouDiag.session"
#define PD_SESSION_ID_LEN 17   /* 16 hex chars + NUL */
#define PD_STEP_ID_MAX     32
#define PD_CONSOLE_ID_MAX  32

typedef enum {
    PD_SESSION_RUNNING = 0,
    PD_SESSION_DONE,
    PD_SESSION_ERROR,
} pd_session_state_t;

typedef struct {
    char                session_id[PD_SESSION_ID_LEN];
    pd_session_state_t  state;
    char                step[PD_STEP_ID_MAX];
    char                console[PD_CONSOLE_ID_MAX]; /* fingerprint, voir core/identity */
    int                 progress_percent;
    char                addons_run[128];             /* liste separee par virgules */
    pd_error_t          last_error;
    pd_result_list_t    results;                      /* resultats intermediaires */
    bool                had_interrupted_session;       /* info pour le rapport/logs */
} pd_session_t;

/* Reprend une session RUNNING existante, ou en cree une nouvelle si
 * aucune / DONE / ERROR / illisible (voir note en tete de fichier).
 * 'console_fingerprint' peut etre NULL si pas encore connu a cet instant
 * (rempli plus tard via pd_session_set_console) - ignore si une session
 * est reprise (le fingerprint deja enregistre est conserve). */
pd_error_t pd_session_start(pd_session_t *session, const char *console_fingerprint);

void pd_session_set_step(pd_session_t *session, const char *step, int progress_percent);
void pd_session_set_console(pd_session_t *session, const char *console_fingerprint);
void pd_session_mark_addon_run(pd_session_t *session, const char *addon_id);
void pd_session_add_result(pd_session_t *session, const char *test,
                            pd_status_t status, const char *source,
                            const char *detail);
void pd_session_set_error(pd_session_t *session, pd_error_t err);

/* Reecrit integralement USB:/PinouDiag.session avec l'etat courant.
 * Appele automatiquement par les setters ci-dessus. */
pd_error_t pd_session_save(pd_session_t *session);

/* A appeler uniquement apres confirmation d'ecriture des rapports finaux
 * (report.json, report.txt, wiimedic.txt). Supprime USB:/PinouDiag.session. */
pd_error_t pd_session_end(pd_session_t *session);

#ifdef __cplusplus
}
#endif

#endif /* PD_SESSION_H */
