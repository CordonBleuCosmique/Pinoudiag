/*
 * PinouDiag Wii V1 - Core / Bootstrap (POC1)
 * [UNVERIFIED - non compile] voir docs/build.md
 */
#ifndef PD_BOOTSTRAP_H
#define PD_BOOTSTRAP_H

#include "../pd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise video, pads/wiimotes, et la console texte.
 * A appeler une seule fois, au tout debut de main(). */
pd_error_t pd_bootstrap_init(void);

/* Lit les boutons pads/wiimote presses depuis le dernier scan (bitmask
 * combinee GC+Wiimote, cf. pd_bootstrap_scan_*). */
void pd_bootstrap_scan_pads(void);
u32  pd_bootstrap_buttons_down(void);

/* Constantes de boutons "logiques" utilisees par PinouDiag, mappees en
 * interne sur WPAD_BUTTON_* / PAD_BUTTON_* (bootstrap.c). */
#define PD_BTN_A     (1 << 0)
#define PD_BTN_B     (1 << 1)
#define PD_BTN_HOME  (1 << 2)

#ifdef __cplusplus
}
#endif

#endif /* PD_BOOTSTRAP_H */
