/* [UNVERIFIED - non compile] voir docs/build.md */
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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

pd_error_t pd_addon_find_dir(const char *addons_root, const char *addon_id,
                              char *out_dir, size_t out_size) {
    DIR *d;
    struct dirent *e;

    d = opendir(addons_root);
    if (!d)
        return PD_ERR_ADDON_NOT_FOUND;

    while ((e = readdir(d)) != NULL) {
        char candidate[PD_ADDON_PATH_MAX];
        struct stat st;
        pd_addon_t addon;

        if (e->d_name[0] == '.')
            continue;

        snprintf(candidate, sizeof(candidate), "%s/%s", addons_root, e->d_name);

        if (stat(candidate, &st) != 0 || !S_ISDIR(st.st_mode))
            continue;

        if (pd_addon_load_manifest(candidate, &addon) != PD_OK)
            continue;

        if (strcmp(addon.id, addon_id) == 0) {
            strncpy(out_dir, candidate, out_size - 1);
            out_dir[out_size - 1] = '\0';
            closedir(d);
            return PD_OK;
        }
    }

    closedir(d);
    return PD_ERR_ADDON_NOT_FOUND;
}

pd_error_t pd_addon_run(const pd_addon_t *addon, const char *addon_dir) {
    char exec_path[PD_ADDON_PATH_MAX];

    snprintf(exec_path, sizeof(exec_path), "%s/%s", addon_dir, addon->executable);

    return pd_dol_chainload(exec_path); /* ne revient qu'en cas d'echec */
}
