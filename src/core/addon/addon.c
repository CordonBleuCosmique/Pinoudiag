/* [UNVERIFIED - non compile] voir docs/build.md */
#include <stdio.h>
#include <string.h>

#include "../json_lite.h"
#include "addon.h"
#include "dol_loader.h"

pd_error_t pd_addon_load_manifest(const char *addon_dir, pd_addon_t *addon) {
    char path[PD_ADDON_PATH_MAX];
    FILE *fp;
    static char buf[4096]; /* static : ne pas grever la pile (motif deja
                             * observe dans WiiMedic report.c, voir
                             * docs/wiimedic.md) */
    size_t n;

    memset(addon, 0, sizeof(*addon));

    snprintf(path, sizeof(path), "%s/manifest.json", addon_dir);

    fp = fopen(path, "r");
    if (!fp)
        return PD_ERR_ADDON_NOT_FOUND;

    n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';

    if (!pd_json_get_string(buf, "id", addon->id, sizeof(addon->id)))
        return PD_ERR_ADDON_LOAD_ERROR;

    pd_json_get_string(buf, "name", addon->name, sizeof(addon->name));
    pd_json_get_string(buf, "version", addon->version, sizeof(addon->version));
    pd_json_get_string(buf, "platform", addon->platform, sizeof(addon->platform));
    pd_json_get_string(buf, "output", addon->output, sizeof(addon->output));

    if (!pd_json_get_string(buf, "executable", addon->executable, sizeof(addon->executable)))
        return PD_ERR_ADDON_LOAD_ERROR;

    return PD_OK;
}

pd_error_t pd_addon_run(const pd_addon_t *addon, const char *addon_dir) {
    char exec_path[PD_ADDON_PATH_MAX];

    snprintf(exec_path, sizeof(exec_path), "%s/%s", addon_dir, addon->executable);

    return pd_dol_chainload(exec_path); /* ne revient qu'en cas d'echec */
}
