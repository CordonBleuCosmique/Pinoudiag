/* [UNVERIFIED - non compile] voir docs/build.md
 *
 * Montage SD/USB via libfat, meme mecanisme bas niveau que fatInitDefault()
 * (utilise par WiiMedic - voir docs/wiimedic.md) mais monte chaque device
 * separement pour pouvoir distinguer leur presence individuellement,
 * necessaire pour appliquer la regle "USB obligatoire, SD lecture seule"
 * du brief (section 5) independamment de ce que fait l'addon.
 */
#include <fat.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <string.h>

#include "storage.h"

pd_error_t pd_storage_init(pd_storage_state_t *state) {
    memset(state, 0, sizeof(*state));

    state->sd_present  = fatMountSimple("sd", &__io_wiisd);
    state->usb_present  = fatMountSimple("usb", &__io_usbstorage);

    if (!state->sd_present && !state->usb_present)
        return PD_ERR_STORAGE_ERROR;

    return PD_OK;
}

void pd_storage_refresh(pd_storage_state_t *state) {
    /* fatMountSimple est idempotent si deja monte (verifie l'interface) ;
     * un re-appel permet de detecter une carte/cle inseree apres le
     * demarrage. [REQUIRES HARDWARE TEST] comportement non verifie si le
     * device est retire puis reinsere sans redemarrage. */
    if (!state->sd_present)
        state->sd_present = fatMountSimple("sd", &__io_wiisd);

    if (!state->usb_present)
        state->usb_present = fatMountSimple("usb", &__io_usbstorage);
}

pd_error_t pd_storage_shutdown(void) {
    /* fatUnmount ferme les handles ouverts et flush les caches du device.
     * On tente les deux independamment de leur etat connu, sans echouer
     * si l'un des deux n'a jamais ete monte. */
    fatUnmount("sd:");
    fatUnmount("usb:");

    return PD_OK;
}
