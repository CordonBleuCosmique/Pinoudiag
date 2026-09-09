/* [UNVERIFIED - non compile] voir docs/build.md et json_lite.h */
#include <stdio.h>
#include <string.h>

#include "json_lite.h"

bool pd_json_get_string(const char *json, const char *key, char *out, size_t outsize) {
    char pattern[64];
    const char *p;
    const char *start;
    const char *end;
    size_t len;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (!p)
        return false;

    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p)
        return false;
    p++;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;

    if (*p != '"')
        return false;
    p++;

    start = p;
    end = strchr(start, '"');
    if (!end)
        return false;

    len = (size_t) (end - start);
    if (len >= outsize)
        len = outsize - 1;

    memcpy(out, start, len);
    out[len] = '\0';

    return true;
}

bool pd_json_find_array(const char *json, const char *key,
                         const char **arr_start, const char **arr_end) {
    char pattern[64];
    const char *p;
    const char *close;

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    p = strstr(json, pattern);
    if (!p)
        return false;

    p = strchr(p, '[');
    if (!p)
        return false;
    p++;

    /* Recherche naive du ']' correspondant : suffisant tant que les
     * objets du tableau ne contiennent pas eux-memes de '[' (vrai pour
     * workflows/*.json, voir en-tete). */
    close = strchr(p, ']');
    if (!close)
        return false;

    *arr_start = p;
    *arr_end = close;

    return true;
}

bool pd_json_next_object(const char **cursor, const char *arr_end,
                          const char **obj_start, const char **obj_end) {
    const char *p = *cursor;
    const char *start;
    int depth;

    while (p < arr_end && *p != '{')
        p++;

    if (p >= arr_end)
        return false;

    start = p + 1;
    depth = 1;
    p++;

    while (p < arr_end && depth > 0) {
        if (*p == '{') depth++;
        else if (*p == '}') depth--;
        if (depth > 0) p++;
    }

    if (depth != 0)
        return false; /* accolade non fermee - manifeste malforme */

    *obj_start = start;
    *obj_end = p;
    *cursor = p + 1;

    return true;
}
