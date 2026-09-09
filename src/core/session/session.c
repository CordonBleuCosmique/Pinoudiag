/* [UNVERIFIED - non compile] voir docs/build.md
 *
 * Format du fichier de session : texte simple ligne par ligne
 * (cle=valeur), choisi plutot qu'un format binaire ou JSON pour rester
 * lisible/diagnosticable a la main (ouvrir le fichier sur un PC suffit)
 * et robuste a un parseur minimal - pas de dependance JSON introduite
 * pour un unique fichier interne au format fixe (brief section 16 :
 * "ne pas introduire de dependances inutiles").
 */
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "session.h"

static void generate_session_id(char *out) {
    /* Pas de source d'entropie materielle dediee identifiee dans le code
     * source lu (WiiMedic/HBC) - on derive un identifiant depuis les
     * ticks systeme, suffisant pour distinguer des sessions successives
     * sur une meme console (pas un identifiant cryptographique).
     * [UNVERIFIED - non teste sur materiel reel] */
    u64 t = gettime();
    snprintf(out, PD_SESSION_ID_LEN, "%08x%08x",
             (unsigned int)(t >> 32), (unsigned int)(t & 0xFFFFFFFF));
}

static bool file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    fclose(f);
    return true;
}

pd_error_t pd_session_start(pd_session_t *session, const char *console_fingerprint) {
    memset(session, 0, sizeof(*session));
    pd_result_list_init(&session->results);

    if (file_exists(PD_SESSION_PATH)) {
        /* Session precedente interrompue (section 6 du brief) : on ne
         * l'ignore jamais silencieusement, on le journalise via le flag
         * pour que le rapport final puisse le mentionner, puis on
         * supprime avant de continuer. */
        session->had_interrupted_session = true;
        remove(PD_SESSION_PATH);
    }

    generate_session_id(session->session_id);
    session->state = PD_SESSION_RUNNING;
    strncpy(session->step, "identify", PD_STEP_ID_MAX - 1);
    session->progress_percent = 0;
    session->last_error = PD_OK;

    if (console_fingerprint)
        strncpy(session->console, console_fingerprint, PD_CONSOLE_ID_MAX - 1);

    return pd_session_save(session);
}

void pd_session_set_step(pd_session_t *session, const char *step, int progress_percent) {
    strncpy(session->step, step, PD_STEP_ID_MAX - 1);
    session->step[PD_STEP_ID_MAX - 1] = '\0';
    session->progress_percent = progress_percent;
    pd_session_save(session);
}

void pd_session_set_console(pd_session_t *session, const char *console_fingerprint) {
    strncpy(session->console, console_fingerprint, PD_CONSOLE_ID_MAX - 1);
    session->console[PD_CONSOLE_ID_MAX - 1] = '\0';
    pd_session_save(session);
}

void pd_session_mark_addon_run(pd_session_t *session, const char *addon_id) {
    size_t len = strlen(session->addons_run);
    size_t remaining = sizeof(session->addons_run) - len;

    if (remaining <= strlen(addon_id) + 2)
        return; /* liste pleine, best effort */

    if (len > 0) {
        strncat(session->addons_run, ",", remaining - 1);
        remaining--;
    }
    strncat(session->addons_run, addon_id, remaining - 1);

    pd_session_save(session);
}

void pd_session_add_result(pd_session_t *session, const char *test,
                            pd_status_t status, const char *source,
                            const char *detail) {
    pd_result_list_add(&session->results, test, status, source, detail);
    pd_session_save(session);
}

void pd_session_set_error(pd_session_t *session, pd_error_t err) {
    session->last_error = err;
    if (err != PD_OK)
        session->state = PD_SESSION_ERROR;
    pd_session_save(session);
}

pd_error_t pd_session_save(pd_session_t *session) {
    FILE *fp;
    int i;
    const char *state_str;

    fp = fopen(PD_SESSION_PATH, "w");
    if (!fp)
        return PD_ERR_SESSION_ERROR;

    switch (session->state) {
        case PD_SESSION_DONE:  state_str = "DONE"; break;
        case PD_SESSION_ERROR: state_str = "ERROR"; break;
        default:                state_str = "RUNNING"; break;
    }

    fprintf(fp, "PINOUDIAG_SESSION v1\n");
    fprintf(fp, "session_id=%s\n", session->session_id);
    fprintf(fp, "state=%s\n", state_str);
    fprintf(fp, "step=%s\n", session->step);
    fprintf(fp, "console=%s\n", session->console);
    fprintf(fp, "progress=%d\n", session->progress_percent);
    fprintf(fp, "addons_run=%s\n", session->addons_run);
    fprintf(fp, "last_error=%s\n", pd_error_str(session->last_error));

    for (i = 0; i < session->results.count; i++) {
        pd_result_t *r = &session->results.items[i];
        fprintf(fp, "result:%s|%s|%s|%s\n",
                r->test, pd_status_str(r->status), r->source, r->detail);
    }

    fflush(fp);
    fclose(fp);

    return PD_OK;
}

pd_error_t pd_session_end(pd_session_t *session) {
    session->state = PD_SESSION_DONE;
    session->progress_percent = 100;

    /* On ecrit une derniere fois l'etat DONE avant suppression : si la
     * suppression echoue ou est interrompue, un lecteur externe du
     * fichier restant verra un etat coherent plutot qu'un RUNNING perime. */
    pd_session_save(session);

    if (remove(PD_SESSION_PATH) != 0)
        return PD_ERR_SESSION_ERROR;

    return PD_OK;
}
