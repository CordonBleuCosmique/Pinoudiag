/* [UNVERIFIED - non compile] voir docs/build.md */
#include "../storage/storage.h"
#include "../../ui/ui.h"
#include "shutdown.h"

pd_error_t pd_shutdown_finalize(pd_session_t *session) {
    pd_error_t err;

    err = pd_session_end(session); /* supprime USB:/PinouDiag.session */
    if (err != PD_OK)
        return err;

    pd_storage_shutdown(); /* flush + demontage SD/USB */

    pd_ui_safe_to_remove();

    return PD_OK;
}
