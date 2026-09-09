/*
 * PinouDiag Wii V1 - Core / Identity (POC7, section 7 du brief)
 * [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * ConsoleFingerprint : voir docs/identity.md pour l'analyse complete des
 * candidats (accessibilite/unicite/stabilite/confidentialite). Retenu :
 * hash(ES_DeviceID || Hollywood revision || boot2 version), jamais les
 * identifiants bruts (le brief section 7 l'exige explicitement).
 *
 * Le hash utilise ici (FNV-1a 64 bits) n'est PAS cryptographique - un
 * fingerprint de diagnostic n'a pas besoin de resistance aux collisions
 * adverses, seulement d'une faible probabilite de collision fortuite
 * entre les Wii d'un meme technicien. Choix documente ici plutot que de
 * reutiliser sha1.c de HBC (GPLv2) pour eviter une dependance et une
 * obligation d'attribution supplementaires pour un besoin non
 * cryptographique (brief section 16 : pas de dependance inutile).
 */
#ifndef PD_FINGERPRINT_H
#define PD_FINGERPRINT_H

#include <stddef.h>

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PD_FINGERPRINT_LEN 13 /* "WII-" + 8 hex + NUL */

/* Calcule le fingerprint et le formate "WII-XXXXXXXX" dans out
 * (out doit faire au moins PD_FINGERPRINT_LEN octets). */
pd_error_t pd_identity_fingerprint(char *out, size_t outsize);

#ifdef __cplusplus
}
#endif

#endif /* PD_FINGERPRINT_H */
