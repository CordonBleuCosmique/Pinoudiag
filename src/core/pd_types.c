/* [UNVERIFIED - non compile] voir docs/build.md */
#include <string.h>

#include "pd_types.h"

const char *pd_error_str(pd_error_t err) {
    switch (err) {
        case PD_OK:                        return "OK";
        case PD_ERR_UNSUPPORTED_WII:        return "UNSUPPORTED_WII";
        case PD_ERR_USB_NOT_FOUND:          return "USB_NOT_FOUND";
        case PD_ERR_STORAGE_ERROR:          return "STORAGE_ERROR";
        case PD_ERR_ADDON_NOT_FOUND:        return "ADDON_NOT_FOUND";
        case PD_ERR_ADDON_LOAD_ERROR:       return "ADDON_LOAD_ERROR";
        case PD_ERR_ADDON_TIMEOUT:          return "ADDON_TIMEOUT";
        case PD_ERR_ADDON_CRASH:            return "ADDON_CRASH";
        case PD_ERR_ADDON_RESULT_MISSING:   return "ADDON_RESULT_MISSING";
        case PD_ERR_RESULT_PARSE_ERROR:     return "RESULT_PARSE_ERROR";
        case PD_ERR_REPORT_WRITE_ERROR:     return "REPORT_WRITE_ERROR";
        case PD_ERR_SESSION_ERROR:          return "SESSION_ERROR";
        default:                            return "UNKNOWN_ERROR";
    }
}

const char *pd_status_str(pd_status_t status) {
    switch (status) {
        case PD_STATUS_PASS:       return "PASS";
        case PD_STATUS_WARN:       return "WARN";
        case PD_STATUS_FAIL:       return "FAIL";
        case PD_STATUS_NOT_TESTED: return "NOT_TESTED";
        case PD_STATUS_ERROR:      return "ERROR";
        default:                   return "UNKNOWN";
    }
}

void pd_result_list_init(pd_result_list_t *list) {
    memset(list, 0, sizeof(*list));
}

bool pd_result_list_add(pd_result_list_t *list, const char *test,
                         pd_status_t status, const char *source,
                         const char *detail) {
    pd_result_t *r;

    if (list->count >= PD_MAX_RESULTS)
        return false;

    r = &list->items[list->count];
    strncpy(r->test, test ? test : "", PD_TEST_NAME_MAX - 1);
    r->status = status;
    strncpy(r->source, source ? source : "", PD_SOURCE_NAME_MAX - 1);
    strncpy(r->detail, detail ? detail : "", PD_TEST_DETAIL_MAX - 1);
    list->count++;

    return true;
}
