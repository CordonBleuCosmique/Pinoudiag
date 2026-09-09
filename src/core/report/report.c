/* [UNVERIFIED - non compile] voir docs/build.md
 *
 * mkdir()/stat() : couche POSIX standard de libfat, pas confirmee par
 * une utilisation directe dans WiiMedic/HBC (aucun des deux ne cree de
 * sous-dossiers) - usage documente comme faisant partie de l'API
 * publique libfat/newlib, a confirmer a la compilation.
 */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "../result/result.h"
#include "report.h"

static void ensure_dir(const char *path) {
    mkdir(path, 0777); /* ignore l'erreur si deja existant */
}

pd_error_t pd_report_prepare_dirs(const char *fingerprint, const char *session_id,
                                   pd_report_paths_t *paths) {
    ensure_dir("usb:/Results");

    snprintf(paths->results_dir, sizeof(paths->results_dir),
             "usb:/Results/%s", fingerprint);
    ensure_dir(paths->results_dir);

    snprintf(paths->session_dir, sizeof(paths->session_dir),
             "%s/%s", paths->results_dir, session_id);
    ensure_dir(paths->session_dir);

    return PD_OK;
}

pd_error_t pd_report_write_identity(const pd_report_paths_t *paths,
                                     const char *fingerprint,
                                     const pd_console_info_t *console_info) {
    char path[PD_REPORT_DIR_MAX + 16];
    FILE *fp;

    snprintf(path, sizeof(path), "%s/identity.json", paths->results_dir);

    fp = fopen(path, "w");
    if (!fp)
        return PD_ERR_REPORT_WRITE_ERROR;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"fingerprint\": \"%s\",\n", fingerprint);
    fprintf(fp, "  \"model\": \"%s\",\n", console_info->model);
    fprintf(fp, "  \"system\": \"%s\"\n", console_info->system);
    fprintf(fp, "}\n");

    fflush(fp);
    fclose(fp);

    return PD_OK;
}

/* Copie brute d'un fichier (lecture par blocs de 512 octets - suffisant
 * pour un rapport texte WiiMedic, quelques Ko). */
static bool copy_file(const char *src, const char *dst, char *content_out, size_t content_out_size) {
    FILE *in, *out;
    char buf[512];
    size_t n;
    size_t total = 0;

    in = fopen(src, "rb");
    if (!in)
        return false;

    out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    if (content_out)
        content_out[0] = '\0';

    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        fwrite(buf, 1, n, out);
        if (content_out && total + n < content_out_size) {
            memcpy(content_out + total, buf, n);
            total += n;
            content_out[total] = '\0';
        }
    }

    fclose(in);
    fflush(out);
    fclose(out);

    return true;
}

pd_error_t pd_report_collect_wiimedic(const pd_report_paths_t *paths,
                                       char *report_text, size_t report_text_size) {
    char dst[PD_REPORT_DIR_MAX + 16];
    /* Chemins en dur cote WiiMedic (voir docs/wiimedic.md, report.c :
     * REPORT_PATH_SD/REPORT_PATH_USB), priorite usb: pour eviter de
     * dependre d'une ecriture SD. */
    static const char *candidates[] = {
        "usb:/WiiMedic_Report.txt",
        "sd:/WiiMedic_Report.txt",
    };
    size_t i;

    snprintf(dst, sizeof(dst), "%s/wiimedic.txt", paths->session_dir);

    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if (copy_file(candidates[i], dst, report_text, report_text_size)) {
            /* Nettoyage : la regle "SD = lecture seule" (section 5) porte
             * sur l'etat final observable, pas sur ce que WiiMedic (hors
             * controle de PinouDiag) ecrit transitoirement - voir
             * docs/usb_sd.md. On supprime la copie SD une fois recuperee. */
            if (strncmp(candidates[i], "sd:", 3) == 0)
                remove(candidates[i]);
            return PD_OK;
        }
    }

    return PD_ERR_ADDON_RESULT_MISSING;
}

static void write_results_json(FILE *fp, const pd_result_list_t *results) {
    int i;

    fprintf(fp, "  \"tests\": [\n");
    for (i = 0; i < results->count; i++) {
        const pd_result_t *r = &results->items[i];
        fprintf(fp, "    { \"test\": \"%s\", \"status\": \"%s\", \"source\": \"%s\" }%s\n",
                r->test, pd_status_str(r->status), r->source,
                (i + 1 < results->count) ? "," : "");
    }
    fprintf(fp, "  ]\n");
}

static pd_status_t worst_status(const pd_result_list_t *results) {
    /* Statut global = le pire des statuts individuels (FAIL > WARN >
     * NOT_TESTED > PASS), ERROR traite comme equivalent a FAIL pour le
     * statut global (mais conserve tel quel par test). */
    bool has_fail = false, has_warn = false, has_error = false;
    int i;

    for (i = 0; i < results->count; i++) {
        switch (results->items[i].status) {
            case PD_STATUS_ERROR: has_error = true; break;
            case PD_STATUS_FAIL:  has_fail = true; break;
            case PD_STATUS_WARN:  has_warn = true; break;
            default: break;
        }
    }

    if (has_error) return PD_STATUS_ERROR;
    if (has_fail)  return PD_STATUS_FAIL;
    if (has_warn)  return PD_STATUS_WARN;
    return PD_STATUS_PASS;
}

pd_error_t pd_report_write_final(const pd_report_paths_t *paths,
                                  const pd_session_t *session,
                                  const pd_console_info_t *console_info,
                                  const char *fingerprint,
                                  const char *pinoudiag_version,
                                  const char *wiimedic_version) {
    char path[PD_REPORT_DIR_MAX + 16];
    FILE *fp;
    int i;
    pd_status_t global = worst_status(&session->results);

    /* --- report.json --- */
    snprintf(path, sizeof(path), "%s/report.json", paths->session_dir);
    fp = fopen(path, "w");
    if (!fp)
        return PD_ERR_REPORT_WRITE_ERROR;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"session_id\": \"%s\",\n", session->session_id);
    fprintf(fp, "  \"fingerprint\": \"%s\",\n", fingerprint);
    fprintf(fp, "  \"model\": \"%s\",\n", console_info->model);
    fprintf(fp, "  \"system\": \"%s\",\n", console_info->system);
    fprintf(fp, "  \"pinoudiag_version\": \"%s\",\n", pinoudiag_version);
    fprintf(fp, "  \"wiimedic_version\": \"%s\",\n", wiimedic_version);
    fprintf(fp, "  \"had_interrupted_session\": %s,\n",
            session->had_interrupted_session ? "true" : "false");
    write_results_json(fp, &session->results);
    fprintf(fp, "  ,\n  \"global_status\": \"%s\"\n", pd_status_str(global));
    fprintf(fp, "}\n");

    fflush(fp);
    fclose(fp);

    /* --- report.txt --- */
    snprintf(path, sizeof(path), "%s/report.txt", paths->session_dir);
    fp = fopen(path, "w");
    if (!fp)
        return PD_ERR_REPORT_WRITE_ERROR;

    fprintf(fp, "========================================\n");
    fprintf(fp, "  PinouDiag Wii V1 - Rapport de diagnostic\n");
    fprintf(fp, "========================================\n\n");
    fprintf(fp, "Console (fingerprint) : %s\n", fingerprint);
    fprintf(fp, "Modele                : %s\n", console_info->model);
    fprintf(fp, "Firmware              : %s\n", console_info->system);
    fprintf(fp, "Session ID            : %s\n", session->session_id);
    fprintf(fp, "Version PinouDiag     : %s\n", pinoudiag_version);
    fprintf(fp, "Version WiiMedic      : %s\n", wiimedic_version);
    if (session->had_interrupted_session)
        fprintf(fp, "\n[ATTENTION] Une session precedente interrompue a ete detectee\n"
                     "et supprimee au demarrage de cette session.\n");
    fprintf(fp, "\n--- Resultats ---\n\n");

    for (i = 0; i < session->results.count; i++) {
        const pd_result_t *r = &session->results.items[i];
        fprintf(fp, "[%-10s] %-20s (%s)%s%s\n",
                pd_status_str(r->status), r->test, r->source,
                r->detail[0] ? " - " : "", r->detail);
    }

    if (session->last_error != PD_OK)
        fprintf(fp, "\n--- Erreur ---\nDerniere erreur : %s\n",
                pd_error_str(session->last_error));

    fprintf(fp, "\n========================================\n");
    fprintf(fp, "Statut global : %s\n", pd_status_str(global));
    fprintf(fp, "========================================\n");

    fflush(fp);
    fclose(fp);

    return PD_OK;
}
