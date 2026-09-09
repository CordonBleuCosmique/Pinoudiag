/* [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * ES_GetDeviceID/SYS_GetHollywoodRevision/ES_GetBoot2Version : usage
 * [VERIFIED] par lecture directe de source/system_info.c (WiiMedic),
 * voir docs/identity.md et docs/wiimedic.md.
 */
#include <gccore.h>
#include <stdio.h>

#include "fingerprint.h"

static u64 fnv1a64(const u8 *data, size_t len) {
    u64 hash = 0xcbf29ce484222325ULL;
    size_t i;

    for (i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 0x100000001b3ULL;
    }

    return hash;
}

pd_error_t pd_identity_fingerprint(char *out, size_t outsize) {
    u32 device_id = 0;
    u32 hollywood_ver;
    u32 boot2_ver = 0;
    u8 buf[12];
    u64 hash;
    u32 short_hash;

    if (ES_GetDeviceID(&device_id) < 0)
        return PD_ERR_UNSUPPORTED_WII;

    hollywood_ver = SYS_GetHollywoodRevision();
    ES_GetBoot2Version(&boot2_ver); /* best effort, 0 si indisponible */

    buf[0]  = (u8) (device_id >> 24);
    buf[1]  = (u8) (device_id >> 16);
    buf[2]  = (u8) (device_id >> 8);
    buf[3]  = (u8) device_id;
    buf[4]  = (u8) (hollywood_ver >> 24);
    buf[5]  = (u8) (hollywood_ver >> 16);
    buf[6]  = (u8) (hollywood_ver >> 8);
    buf[7]  = (u8) hollywood_ver;
    buf[8]  = (u8) (boot2_ver >> 24);
    buf[9]  = (u8) (boot2_ver >> 16);
    buf[10] = (u8) (boot2_ver >> 8);
    buf[11] = (u8) boot2_ver;

    hash = fnv1a64(buf, sizeof(buf));
    short_hash = (u32) (hash >> 32); /* moitie haute, meilleure dispersion que le low */

    snprintf(out, outsize, "WII-%08X", short_hash);

    return PD_OK;
}
