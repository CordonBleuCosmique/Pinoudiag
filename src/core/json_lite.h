/*
 * PinouDiag Wii V1 - Core / lecteur JSON minimal
 * [UNVERIFIED - non compile] voir docs/build.md
 *
 * Volontairement pas un moteur JSON general : les schemas consommes
 * (manifest.json d'un addon, workflows/*.json) sont fixes et connus
 * d'avance - un lecteur cle/valeur + iteration de tableau plat suffit et
 * evite une dependance JSON generale inutile (brief section 16).
 * Ne gere ni l'imbrication profonde, ni les echappements, ni les types
 * numeriques/booleens - uniquement ce dont PinouDiag a besoin.
 */
#ifndef PD_JSON_LITE_H
#define PD_JSON_LITE_H

#include <stddef.h>

#include <gctypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Cherche "key": "valeur" n'importe ou dans json (premiere occurrence) et
 * copie valeur (sans guillemets) dans out. */
bool pd_json_get_string(const char *json, const char *key, char *out, size_t outsize);

/* Localise le contenu d'un tableau "key": [ ... ] et renvoie des
 * pointeurs sur le premier caractere apres '[' et le caractere ']'
 * correspondant (recherche naive du premier ']' apres le '[' - suffisant
 * tant que les objets de ce tableau ne contiennent pas eux-memes de
 * tableau, ce qui est le cas de workflows/*.json). */
bool pd_json_find_array(const char *json, const char *key,
                         const char **arr_start, const char **arr_end);

/* Itere les objets {...} de premier niveau dans [arr_start, arr_end).
 * *cursor doit valoir arr_start au premier appel ; l'appel met a jour
 * *cursor pour l'iteration suivante et renvoie false quand il n'y a plus
 * d'objet. obj_start/obj_end delimitent l'objet courant (guillemets
 * inclus autour des cles, accolades exclues). */
bool pd_json_next_object(const char **cursor, const char *arr_end,
                          const char **obj_start, const char **obj_end);

#ifdef __cplusplus
}
#endif

#endif /* PD_JSON_LITE_H */
