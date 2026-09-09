/* [UNVERIFIED - non compile] voir docs/build.md */
#include <stdio.h>
#include <string.h>

#include "../json_lite.h"
#include "workflow.h"

pd_error_t pd_workflow_load(const char *path, pd_workflow_t *wf) {
    FILE *fp;
    static char buf[4096]; /* static : voir motif deja utilise dans core/addon */
    size_t n;
    const char *arr_start, *arr_end, *cursor;
    const char *obj_start, *obj_end;
    char obj[256];

    memset(wf, 0, sizeof(*wf));

    fp = fopen(path, "r");
    if (!fp)
        return PD_ERR_ADDON_NOT_FOUND; /* pas de code d'erreur dedie "workflow introuvable" */

    n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';

    pd_json_get_string(buf, "id", wf->id, sizeof(wf->id));
    pd_json_get_string(buf, "name", wf->name, sizeof(wf->name));

    if (!pd_json_find_array(buf, "steps", &arr_start, &arr_end))
        return PD_ERR_RESULT_PARSE_ERROR;

    cursor = arr_start;
    wf->step_count = 0;

    while (wf->step_count < PD_WORKFLOW_MAX_STEPS &&
           pd_json_next_object(&cursor, arr_end, &obj_start, &obj_end)) {
        pd_workflow_step_t *step = &wf->steps[wf->step_count];
        size_t len = (size_t) (obj_end - obj_start);
        char type_str[16];

        if (len >= sizeof(obj))
            len = sizeof(obj) - 1;
        memcpy(obj, obj_start, len);
        obj[len] = '\0';

        memset(step, 0, sizeof(*step));

        if (!pd_json_get_string(obj, "id", step->id, sizeof(step->id)))
            continue; /* etape sans id : ignoree plutot que corrompre le workflow */

        if (!pd_json_get_string(obj, "type", type_str, sizeof(type_str)))
            return PD_ERR_RESULT_PARSE_ERROR; /* section 9 : "valide les etapes" */

        if (strcmp(type_str, "internal") == 0) {
            step->type = PD_STEP_TYPE_INTERNAL;
        } else if (strcmp(type_str, "addon") == 0) {
            step->type = PD_STEP_TYPE_ADDON;
            if (!pd_json_get_string(obj, "addon", step->addon, sizeof(step->addon)))
                return PD_ERR_RESULT_PARSE_ERROR; /* etape addon sans addon declare */
        } else {
            return PD_ERR_RESULT_PARSE_ERROR; /* type inconnu */
        }

        wf->step_count++;
    }

    if (wf->step_count == 0)
        return PD_ERR_RESULT_PARSE_ERROR;

    return PD_OK;
}

int pd_workflow_find_index(const pd_workflow_t *wf, const char *step_id) {
    int i;

    for (i = 0; i < wf->step_count; i++) {
        if (strcmp(wf->steps[i].id, step_id) == 0)
            return i;
    }

    return -1;
}
