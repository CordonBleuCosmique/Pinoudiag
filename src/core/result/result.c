/*
 * [UNVERIFIED - non compile] voir docs/build.md et result.h
 *
 * Chaque regle ci-dessous cite le fichier source WiiMedic qui produit la
 * ligne recherchee (clone local lu pendant la recherche, voir
 * docs/wiimedic.md). Statut de confiance indique par regle - certaines
 * chaines n'ont pu etre confirmees que jusqu'a l'appel ui_draw_*() sans
 * tracer si elles sont bien recopiees dans le buffer persiste (s_report)
 * - marquees [UNVERIFIED] en consequence, a corriger si un rapport reel
 * genere par WiiMedic infirme la regle (le texte original est toujours
 * conserve tel quel a cote, voir core/report).
 */
#include <stdio.h>
#include <string.h>

#include "result.h"

/* Trouve la premiere ligne qui commence par 'prefix' (apres avoir
 * ignore les espaces de tete) et copie ce qui suit (trim), sur une
 * seule ligne, dans out. Renvoie false si non trouve. */
static bool find_line_value(const char *text, const char *prefix, char *out, size_t outsize) {
    const char *p = text;
    size_t plen = strlen(prefix);

    while ((p = strstr(p, prefix)) != NULL) {
        /* s'assurer qu'on est en debut de ligne (ou tout debut du texte) */
        if (p == text || *(p - 1) == '\n') {
            const char *val = p + plen;
            const char *eol = strchr(val, '\n');
            size_t len;

            while (*val == ' ' || *val == '\t')
                val++;

            len = eol ? (size_t) (eol - val) : strlen(val);
            if (len >= outsize)
                len = outsize - 1;

            memcpy(out, val, len);
            out[len] = '\0';

            /* retire un eventuel '\r' de fin (fichiers texte Wii/FAT) */
            if (len > 0 && out[len - 1] == '\r')
                out[len - 1] = '\0';

            return true;
        }
        p += plen;
    }

    return false;
}

static bool contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL;
}

int pd_result_parse_wiimedic_report(const char *report_text, pd_result_list_t *out) {
    char val[128];
    int added = 0;

    /* [VERIFIED] source_info.c:396-400,446-448,468 - ligne exacte
     * "Protection Rating:   GOOD|PARTIAL|NONE" dans get_system_info_report(). */
    if (find_line_value(report_text, "Protection Rating:", val, sizeof(val))) {
        pd_status_t st = contains(val, "GOOD") ? PD_STATUS_PASS :
                          contains(val, "PARTIAL") ? PD_STATUS_WARN :
                          PD_STATUS_FAIL;
        pd_result_list_add(out, "brick_protection", st, "wiimedic", val);
        added++;
    }

    /* [VERIFIED] nand_health.c:186-229 - ligne "Status:  GOOD|FAIR - ...|POOR - ..."
     * dans get_nand_health_report(). */
    if (find_line_value(report_text, "Status:", val, sizeof(val))) {
        pd_status_t st = contains(val, "GOOD") ? PD_STATUS_PASS :
                          contains(val, "FAIR") ? PD_STATUS_WARN :
                          contains(val, "POOR") ? PD_STATUS_FAIL :
                          PD_STATUS_NOT_TESTED;
        pd_result_list_add(out, "nand_health", st, "wiimedic", val);
        added++;
    }

    /* [VERIFIED] ios_check.c:196-199 - ligne "Total: X | Active: X | Stubs: X | cIOS: N".
     * Heuristique : cIOS present (N>0) -> PASS, absent -> WARN (message
     * explicite de WiiMedic : "USB loaders won't work without one"). */
    {
        const char *p = strstr(report_text, "cIOS:");
        if (p) {
            int n = -1;
            sscanf(p, "cIOS: %d", &n);
            if (n >= 0) {
                snprintf(val, sizeof(val), "cIOS: %d", n);
                pd_result_list_add(out, "ios_scan",
                                    n > 0 ? PD_STATUS_PASS : PD_STATUS_WARN,
                                    "wiimedic", val);
                added++;
            }
        }
    }

    /* [VERIFIED] storage_test.c:213-237 - lignes exactes "SD Card: Present,
     * benchmark completed" / "SD Card: Not present" / "USB: Present, ..." /
     * "USB: Not present" dans run_storage_test() (s_report). Pas de score
     * de vitesse dans le texte persiste (seulement affiche a l'ecran) -
     * NOT_TESTED plutot que FAIL quand le device est absent, ce n'est pas
     * un echec du support teste. */
    if (contains(report_text, "SD Card: Present, benchmark completed")) {
        pd_result_list_add(out, "storage_sd", PD_STATUS_PASS, "wiimedic",
                            "SD Card: Present, benchmark completed");
        added++;
    } else if (contains(report_text, "SD Card: Not present")) {
        pd_result_list_add(out, "storage_sd", PD_STATUS_NOT_TESTED, "wiimedic",
                            "SD Card: Not present");
        added++;
    }

    if (contains(report_text, "USB: Present, benchmark completed")) {
        pd_result_list_add(out, "storage_usb", PD_STATUS_PASS, "wiimedic",
                            "USB: Present, benchmark completed");
        added++;
    } else if (contains(report_text, "USB: Not present")) {
        pd_result_list_add(out, "storage_usb", PD_STATUS_NOT_TESTED, "wiimedic",
                            "USB: Not present");
        added++;
    }

    /* [UNVERIFIED] network_test.c:307-313 - "Internet: FULL connectivity" /
     * "PARTIAL" / "NONE" sont ecrites via ui_draw_ok/warn/err ; le tracage
     * complet de run_network_test() (300+ lignes) n'a pas confirme dans
     * cette session que ce texte exact est recopie dans s_report (le
     * buffer persiste vers le fichier) plutot que seulement affiche a
     * l'ecran. Si absent du rapport reel, cette regle ne matchera
     * simplement rien (pas de faux resultat) - a corriger avec un
     * rapport WiiMedic reel en main. */
    if (contains(report_text, "Internet: FULL"))
        pd_result_list_add(out, "network", PD_STATUS_PASS, "wiimedic", "Internet: FULL connectivity");
    else if (contains(report_text, "Internet: PARTIAL"))
        pd_result_list_add(out, "network", PD_STATUS_WARN, "wiimedic", "Internet: PARTIAL");
    else if (contains(report_text, "Internet: NONE"))
        pd_result_list_add(out, "network", PD_STATUS_FAIL, "wiimedic", "Internet: NONE");
    else
        pd_result_list_add(out, "network", PD_STATUS_NOT_TESTED, "wiimedic", "");
    added++;

    /* [UNVERIFIED] controller_test.c - aucun mot-cle de statut fiable
     * identifie dans le texte persiste (contenu surtout descriptif :
     * extension detectee, etat batterie). Best effort : la section
     * "=== CONTROLLER DIAGNOSTICS ===" presente et non vide -> PASS
     * (au sens "test execute"), absente -> NOT_TESTED. Ne distingue pas
     * un probleme materiel reel detecte par WiiMedic (ex. stick drift) -
     * limitation a lever en lisant un rapport reel. */
    if (contains(report_text, "=== CONTROLLER DIAGNOSTICS ==="))
        pd_result_list_add(out, "controllers", PD_STATUS_PASS, "wiimedic",
                            "section presente (detail non analyse)");
    else
        pd_result_list_add(out, "controllers", PD_STATUS_NOT_TESTED, "wiimedic", "");
    added++;

    return added;
}
