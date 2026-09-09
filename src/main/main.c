/*
 * PinouDiag Wii V1 - point d'entree
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * POC1 (bootstrap) + POC2 (storage) : initialise l'affichage/les pads,
 * detecte l'environnement, monte SD/USB, affiche un ecran d'etat.
 * Les etapes suivantes (session, addon, workflow, rapport - POC3 a POC7)
 * seront branchees ici au fur et a mesure de leur implementation.
 */
#include <gccore.h>
#include <stdio.h>
#include <unistd.h>

#include "../core/bootstrap/bootstrap.h"
#include "../core/detection/detection.h"
#include "../core/pd_types.h"
#include "../core/storage/storage.h"
#include "../ui/ui.h"

int main(int argc, char **argv) {
    pd_console_info_t  console_info;
    pd_storage_state_t storage_state;
    pd_error_t          err;

    pd_bootstrap_init();

    err = pd_detection_check(&console_info);
    if (err != PD_OK) {
        pd_ui_banner(NULL, NULL);
        pd_ui_error(err, "environnement non compatible V1 (Wii uniquement)");
        /* [UNVERIFIED] pas de retour au loader defini a ce stade (POC4) ;
         * on boucle en attendant une action utilisateur pour eviter un
         * ecran fige sans information. */
        for (;;) {
            pd_bootstrap_scan_pads();
            VIDEO_WaitVSync();
        }
    }

    err = pd_storage_init(&storage_state);

    pd_ui_banner(console_info.model, console_info.system);
    pd_ui_info("PinouDiag OK");
    printf("\n");
    printf("SD  : %s (lecture seule)\n", storage_state.sd_present ? "detectee" : "absente");
    printf("USB : %s\n", storage_state.usb_present ? "detectee" : "absente");

    if (err != PD_OK) {
        pd_ui_error(PD_ERR_STORAGE_ERROR, "aucun support de stockage detecte");
    } else if (!storage_state.usb_present) {
        /* Section 14 du brief : l'absence d'USB est geree explicitement,
         * pas comme une erreur bloquante - les etapes qui necessitent une
         * ecriture persistante (session, Results/) seront implementees en
         * POC3 avec cette contrainte. */
        pd_ui_error(PD_ERR_USB_NOT_FOUND,
                    "aucun rapport persistant ne pourra etre sauvegarde");
    }

    /* Fin POC1/POC2 : boucle d'attente. A remplacer par l'enchainement
     * session -> workflow -> addon des POC suivants. */
    for (;;) {
        pd_bootstrap_scan_pads();
        if (pd_bootstrap_buttons_down() & PD_BTN_HOME)
            break;
        VIDEO_WaitVSync();
    }

    pd_storage_shutdown();

    return 0;
}
