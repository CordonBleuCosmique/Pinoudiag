/*
 * PinouDiag Wii V1 - Core / Report (POC7, sections 5 et 11 du brief)
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Structure USB cible (section 5) :
 *   USB:/Results/WII-XXXXXXXX/identity.json
 *   USB:/Results/WII-XXXXXXXX/<SESSION-ID>/report.json
 *   USB:/Results/WII-XXXXXXXX/<SESSION-ID>/report.txt
 *   USB:/Results/WII-XXXXXXXX/<SESSION-ID>/wiimedic.txt
 * Jamais d'ecrasement des anciens diagnostics (chaque session a son
 * propre sous-dossier horodate par session_id).
 */
#ifndef PD_REPORT_H
#define PD_REPORT_H

#include "../detection/detection.h"
#include "../pd_types.h"
#include "../session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PD_REPORT_DIR_MAX 128

typedef struct {
    char results_dir[PD_REPORT_DIR_MAX];   /* USB:/Results/<fingerprint> */
    char session_dir[PD_REPORT_DIR_MAX];   /* .../<session_id> */
} pd_report_paths_t;

/* Cree l'arborescence USB:/Results/<fingerprint>/<session_id>/ (mkdir
 * recursif best-effort) et remplit paths. */
pd_error_t pd_report_prepare_dirs(const char *fingerprint, const char *session_id,
                                   pd_report_paths_t *paths);

/* Ecrit (ou reecrit - contenu statique par console) identity.json. */
pd_error_t pd_report_write_identity(const pd_report_paths_t *paths,
                                     const char *fingerprint,
                                     const pd_console_info_t *console_info);

/* Cherche WiiMedic_Report.txt sur usb: puis sd: (voir docs/usb_sd.md),
 * le copie vers <session_dir>/wiimedic.txt, puis supprime la copie
 * source si elle etait sur sd: (SD = lecture seule a l'etat final,
 * meme si WiiMedic y a transitoirement ecrit - voir docs/usb_sd.md).
 * Remplit report_text (buffer fourni par l'appelant) avec le contenu
 * pour que core/result puisse le parser sans relire le fichier. */
pd_error_t pd_report_collect_wiimedic(const pd_report_paths_t *paths,
                                       char *report_text, size_t report_text_size);

/* Genere report.json et report.txt dans paths->session_dir a partir de
 * l'etat de session (resultats deja accumules via pd_session_add_result). */
pd_error_t pd_report_write_final(const pd_report_paths_t *paths,
                                  const pd_session_t *session,
                                  const pd_console_info_t *console_info,
                                  const char *fingerprint,
                                  const char *pinoudiag_version,
                                  const char *wiimedic_version);

#ifdef __cplusplus
}
#endif

#endif /* PD_REPORT_H */
