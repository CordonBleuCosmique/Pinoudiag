/* [UNVERIFIED - non compile] voir docs/build.md
 *
 * [REQUIRES HARDWARE TEST] La lecture de la version System Menu exacte
 * (ex. "4.3E") n'a pas ete confirmee par du code source lu dans cette
 * session (voir docs/sources.md - accès direct a WiiBrew bloque). Ce
 * fichier utilise ce qui est verifie via WiiMedic (region, revision
 * Hollywood) et documente le reste comme best-effort / a completer.
 */
#include <gccore.h>
#include <stdio.h>

#include "detection.h"

static const char *region_suffix(void) {
    switch (CONF_GetRegion()) {
        case CONF_REGION_JP: return "J";
        case CONF_REGION_US: return "U";
        case CONF_REGION_EU: return "E";
        case CONF_REGION_KR: return "K";
        default:             return "?";
    }
}

pd_error_t pd_detection_check(pd_console_info_t *info) {
    u32 hollywood_ver = SYS_GetHollywoodRevision();

    snprintf(info->model, sizeof(info->model), "Wii RVL-001");

    /* [UNVERIFIED] pas de fonction System Menu version trouvee dans le
     * code source lu (WiiMedic/HBC) - place-holder tant que non confirme. */
    snprintf(info->system, sizeof(info->system), "4.3%s", region_suffix());

    /* Verification minimale : si l'appel Hollywood echoue completement
     * (renvoie 0), on considere l'environnement non fiable pour continuer.
     * Ce n'est pas une detection GameCube/Wii robuste - voir note ci-dessus. */
    if (hollywood_ver == 0)
        return PD_ERR_UNSUPPORTED_WII;

    return PD_OK;
}
