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

/* Recharge le fichier de session tel qu'ecrit par pd_session_save().
 * Renvoie false si le fichier est absent, illisible, ou visiblement
 * corrompu (premiere ligne inattendue). */
static bool session_load(pd_session_t *session) {
    FILE *fp;
    char line[300];

    fp = fopen(PD_SESSION_PATH, "r");
    if (!fp)
        return false;

    memset(session, 0, sizeof(*session));
    pd_result_list_init(&session->results);

    if (!fgets(line, sizeof(line), fp) || strncmp(line, "PINOUDIAG_SESSION", 17) != 0) {
        fclose(fp);
        return false;
    }

    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        char *eq;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (strncmp(line, "result:", 7) == 0) {
            /* format : result:<test>|<status>|<source>|<detail> */
            char *test, *status_str, *source, *detail;
            char *p = line + 7;

            test = p;
            p = strchr(p, '|'); if (!p) continue; *p++ = '\0';
            status_str = p;
            p = strchr(p, '|'); if (!p) continue; *p++ = '\0';
            source = p;
            p = strchr(p, '|'); if (!p) continue; *p++ = '\0';
            detail = p;

            {
                pd_status_t st = PD_STATUS_NOT_TESTED;
                if      (strcmp(status_str, "PASS") == 0)       st = PD_STATUS_PASS;
                else if (strcmp(status_str, "WARN") == 0)       st = PD_STATUS_WARN;
                else if (strcmp(status_str, "FAIL") == 0)       st = PD_STATUS_FAIL;
                else if (strcmp(status_str, "ERROR") == 0)      st = PD_STATUS_ERROR;
                pd_result_list_add(&session->results, test, st, source, detail);
            }
            continue;
        }

        eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        if (strcmp(line, "session_id") == 0)
            strncpy(session->session_id, eq + 1, PD_SESSION_ID_LEN - 1);
        else if (strcmp(line, "state") == 0) {
            if (strcmp(eq + 1, "DONE") == 0) session->state = PD_SESSION_DONE;
            else if (strcmp(eq + 1, "ERROR") == 0) session->state = PD_SESSION_ERROR;
            else session->state = PD_SESSION_RUNNING;
        } else if (strcmp(line, "step") == 0)
            strncpy(session->step, eq + 1, PD_STEP_ID_MAX - 1);
        else if (strcmp(line, "console") == 0)
            strncpy(session->console, eq + 1, PD_CONSOLE_ID_MAX - 1);
        else if (strcmp(line, "progress") == 0)
            session->progress_percent = atoi(eq + 1);
        else if (strcmp(line, "addons_run") == 0)
            strncpy(session->addons_run, eq + 1, sizeof(session->addons_run) - 1);
        /* last_error n'est pas reparse (chaine libre, pas critique pour
         * la reprise - re-derive via l'execution normale du workflow) */
    }

    fclose(fp);
    return true;
}

pd_error_t pd_session_start(pd_session_t *session, const char *console_fingerprint) {
    if (session_load(session)) {
        if (session->state == PD_SESSION_RUNNING) {
            /* Reprise legitime - voir note en tete de session.h sur la
             * necessite de cette exception par rapport au texte litteral
             * du brief, requise par l'Option A (docs/return_to_loader.md). */
            return PD_OK;
        }
        /* DONE/ERROR : traite comme interrompu ci-dessous. */
    }

    memset(session, 0, sizeof(*session));
    pd_result_list_init(&session->results);

    /* Fichier absent, illisible, ou DONE/ERROR resultant : journalise
     * comme interrompu (section 6 du brief) avant d'en creer un nouveau. */
    {
        FILE *f = fopen(PD_SESSION_PATH, "r");
        if (f) {
            fclose(f);
            session->had_interrupted_session = true;
            remove(PD_SESSION_PATH);
        }
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
