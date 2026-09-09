/*
 * PinouDiag Wii V1 - point d'entree (POC7 : pipeline complet)
 * [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * Orchestration de haut niveau : charge workflows/wii_v1.json, execute
 * chaque etape (interne ou addon), sauvegarde l'etat de session a
 * chaque transition, et gere la reprise apres le chainload aller-retour
 * vers/depuis un addon (voir docs/return_to_loader.md - Option A).
 *
 * Les etapes "internal" (identify/storage/analysis/report) sont
 * implementees ici : c'est le point naturel d'orchestration concrete,
 * le Core (core/workflow) restant lui generique (voir workflow.h). Les
 * etapes "addon" sont entierement generiques (core/addon, manifeste).
 */
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../core/addon/addon.h"
#include "../core/bootstrap/bootstrap.h"
#include "../core/detection/detection.h"
#include "../core/identity/fingerprint.h"
#include "../core/pd_types.h"
#include "../core/report/report.h"
#include "../core/result/result.h"
#include "../core/session/session.h"
#include "../core/shutdown/shutdown.h"
#include "../core/storage/storage.h"
#include "../core/workflow/workflow.h"
#include "../ui/ui.h"

#define PD_VERSION        "1.0.0"
#define PD_WORKFLOW_PATH  "sd:/PinouDiag/workflows/wii_v1.json"
#define PD_ADDONS_ROOT    "sd:/PinouDiag/addons"

static char s_wiimedic_report_text[8192];
static char s_wiimedic_version[PD_ADDON_VERSION_MAX] = "inconnue";

static void fatal_loop(pd_error_t err, const char *context) {
    pd_ui_error(err, context);
    for (;;) {
        pd_bootstrap_scan_pads();
        VIDEO_WaitVSync();
    }
}

/* --- Etapes internes --- */

static pd_error_t step_identify(pd_session_t *session, pd_console_info_t *console_info) {
    char fp[PD_FINGERPRINT_LEN];
    pd_error_t err = pd_identity_fingerprint(fp, sizeof(fp));

    if (err != PD_OK)
        return err;

    pd_session_set_console(session, fp);
    pd_ui_info(console_info->model);

    return PD_OK;
}

static pd_error_t step_storage(pd_session_t *session, pd_storage_state_t *storage) {
    pd_storage_refresh(storage);

    if (!storage->usb_present) {
        /* Section 14 du brief : ne bloque pas les etapes qui n'ont pas
         * besoin de l'USB, mais l'indique clairement - SAFE_TO_REMOVE
         * (core/shutdown) ne sera de toute facon jamais atteint tant que
         * des ecritures necessaires restent en attente. */
        pd_session_set_error(session, PD_ERR_USB_NOT_FOUND);
        pd_ui_error(PD_ERR_USB_NOT_FOUND, "aucun rapport persistant ne pourra etre sauvegarde");
        return PD_ERR_USB_NOT_FOUND;
    }

    return PD_OK;
}

/* resume_step_id/resume_percent : l'etape ou reprendre APRES cet addon
 * (l'etape suivante du workflow, ou l'etape courante si c'est la
 * derniere). Doit etre ecrit dans la session AVANT le chainload,
 * puisqu'un succes ne revient jamais ici (voir docs/return_to_loader.md) -
 * sans cette pre-avance, une reprise apres addon relancerait l'addon en
 * boucle infinie au lieu de passer a l'etape suivante. */
static pd_error_t step_run_addon(pd_session_t *session, const char *addon_id,
                                  const char *resume_step_id, int resume_percent) {
    char addon_dir[PD_ADDON_PATH_MAX];
    pd_addon_t addon;
    pd_error_t err;

    err = pd_addon_find_dir(PD_ADDONS_ROOT, addon_id, addon_dir, sizeof(addon_dir));
    if (err != PD_OK) {
        pd_session_set_error(session, PD_ERR_ADDON_NOT_FOUND);
        return err;
    }

    err = pd_addon_load_manifest(addon_dir, &addon);
    if (err != PD_OK) {
        pd_session_set_error(session, err);
        return err;
    }

    pd_session_mark_addon_run(session, addon.id);
    pd_session_set_step(session, resume_step_id, resume_percent);
    pd_ui_busy(addon.name, "Diagnostic en cours...");

    /* Ne revient qu'en cas d'echec de chargement - en cas de succes,
     * l'execution continue dans l'addon puis, a sa sortie, dans le
     * second chainload qui recharge sd:/boot.dol (voir CHANGES.md du
     * fork WiiMedic). Cette fonction main() ne "voit" jamais ce retour :
     * c'est un nouveau demarrage complet de PinouDiag, repris plus haut
     * via pd_session_start() - a l'etape resume_step_id deja enregistree
     * ci-dessus. */
    err = pd_addon_run(&addon, addon_dir);
    pd_session_set_error(session, err);

    return err;
}

static pd_error_t step_analysis(pd_session_t *session) {
    pd_report_paths_t paths;
    pd_error_t err;

    err = pd_report_prepare_dirs(session->console, session->session_id, &paths);
    if (err != PD_OK)
        return err;

    err = pd_report_collect_wiimedic(&paths, s_wiimedic_report_text,
                                      sizeof(s_wiimedic_report_text));
    if (err != PD_OK) {
        /* Pas de rapport WiiMedic recupere : on le journalise comme
         * resultat en erreur plutot que de faire echouer tout le
         * workflow (section 13 : une erreur n'est jamais interpretee
         * comme un succes, mais elle ne doit pas non plus bloquer la
         * generation d'un rapport partiel - section 11). */
        pd_session_add_result(session, "wiimedic_report", PD_STATUS_ERROR,
                               "pinoudiag", "WiiMedic_Report.txt introuvable sur SD/USB");
        pd_session_set_error(session, err);
        return PD_OK; /* on continue vers l'etape rapport avec ce qu'on a */
    }

    pd_result_parse_wiimedic_report(s_wiimedic_report_text, &session->results);
    pd_session_save(session);

    return PD_OK;
}

static pd_error_t step_report(pd_session_t *session, const pd_console_info_t *console_info) {
    pd_report_paths_t paths;
    pd_error_t err;
    char addon_dir[PD_ADDON_PATH_MAX];
    pd_addon_t addon;

    /* Version de l'addon executee, pour l'inclure dans le rapport
     * (section 11). Best-effort : si le manifeste n'est plus lisible a
     * ce stade, on garde "inconnue" plutot que d'echouer le rapport. */
    if (pd_addon_find_dir(PD_ADDONS_ROOT, "wiimedic", addon_dir, sizeof(addon_dir)) == PD_OK &&
        pd_addon_load_manifest(addon_dir, &addon) == PD_OK)
        strncpy(s_wiimedic_version, addon.version, sizeof(s_wiimedic_version) - 1);

    err = pd_report_prepare_dirs(session->console, session->session_id, &paths);
    if (err != PD_OK)
        return err;

    err = pd_report_write_identity(&paths, session->console, console_info);
    if (err != PD_OK)
        return err;

    return pd_report_write_final(&paths, session, console_info, session->console,
                                  PD_VERSION, s_wiimedic_version);
}

int main(int argc, char **argv) {
    pd_console_info_t  console_info;
    pd_storage_state_t storage_state;
    pd_workflow_t       workflow;
    pd_session_t        session;
    pd_error_t          err;
    int                 start_index, i;

    pd_bootstrap_init();

    err = pd_detection_check(&console_info);
    if (err != PD_OK)
        fatal_loop(err, "environnement non compatible V1 (Wii uniquement)");

    pd_storage_init(&storage_state); /* etat affine par l'etape "storage" du workflow */

    err = pd_workflow_load(PD_WORKFLOW_PATH, &workflow);
    if (err != PD_OK)
        fatal_loop(err, "workflow introuvable ou invalide (" PD_WORKFLOW_PATH ")");

    err = pd_session_start(&session, NULL);
    if (err != PD_OK)
        fatal_loop(err, "impossible d'initialiser la session (USB absente ?)");

    pd_ui_banner(console_info.model, console_info.system);
    pd_ui_progress_begin();
    if (session.had_interrupted_session)
        pd_ui_info("(session precedente interrompue detectee et effacee)");

    start_index = pd_workflow_find_index(&workflow, session.step);
    if (start_index < 0)
        start_index = 0;

    for (i = 0; i < workflow.step_count; i++) {
        pd_step_state_t ui_state = (i < start_index) ? PD_STEP_DONE :
                                    (i == start_index) ? PD_STEP_CURRENT : PD_STEP_PENDING;
        pd_ui_progress_step(workflow.steps[i].id, ui_state);
    }
    pd_ui_progress_bar((start_index * 100) / (workflow.step_count > 0 ? workflow.step_count : 1));

    for (i = start_index; i < workflow.step_count; i++) {
        pd_workflow_step_t *step = &workflow.steps[i];

        pd_ui_progress_step(step->id, PD_STEP_CURRENT);

        if (step->type == PD_STEP_TYPE_ADDON) {
            bool has_next = (i + 1 < workflow.step_count);
            const char *resume_id = has_next ? workflow.steps[i + 1].id : step->id;
            int resume_pct = ((has_next ? i + 1 : i) * 100) / workflow.step_count;

            err = step_run_addon(&session, step->addon, resume_id, resume_pct);
            /* Si on arrive ici, pd_addon_run a echoue (sinon: chainload,
             * jamais de retour). On journalise et on continue quand meme
             * vers l'analyse/rapport avec ce qu'on a (section 13). */
            pd_ui_progress_step(step->id, PD_STEP_ERROR);
            continue;
        }

        pd_session_set_step(&session, step->id, (i * 100) / workflow.step_count);

        if (strcmp(step->id, "identify") == 0)
            err = step_identify(&session, &console_info);
        else if (strcmp(step->id, "storage") == 0)
            err = step_storage(&session, &storage_state);
        else if (strcmp(step->id, "analysis") == 0)
            err = step_analysis(&session);
        else if (strcmp(step->id, "report") == 0)
            err = step_report(&session, &console_info);
        else
            err = PD_OK; /* etape interne inconnue : ignoree, deja validee au chargement */

        pd_ui_progress_step(step->id, (err == PD_OK) ? PD_STEP_DONE : PD_STEP_ERROR);
    }

    pd_ui_progress_end();

    /* SAFE_TO_REMOVE (section 15) : uniquement si aucune ecriture requise
     * ne reste en attente - ici, si l'USB etait absente, report.json/txt
     * n'ont pas pu etre ecrits et on ne pretend pas que c'est termine. */
    if (!storage_state.usb_present) {
        pd_ui_error(PD_ERR_USB_NOT_FOUND, "resultats non sauvegardes - SAFE_TO_REMOVE non atteint");
        for (;;) {
            pd_bootstrap_scan_pads();
            VIDEO_WaitVSync();
        }
    }

    pd_shutdown_finalize(&session);

    for (;;) {
        pd_bootstrap_scan_pads();
        VIDEO_WaitVSync();
    }

    return 0;
}
