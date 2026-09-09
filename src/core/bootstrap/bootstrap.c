/* [UNVERIFIED - non compile] voir docs/build.md */
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "../../ui/ui.h"
#include "bootstrap.h"

static u32 s_buttons_down;

pd_error_t pd_bootstrap_init(void) {
    pd_ui_init();

    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
    PAD_Init();

    s_buttons_down = 0;

    return PD_OK;
}

void pd_bootstrap_scan_pads(void) {
    u32 wpad, gpad;

    WPAD_ScanPads();
    PAD_ScanPads();

    wpad = WPAD_ButtonsDown(0);
    gpad = PAD_ButtonsDown(0);

    s_buttons_down = 0;
    if ((wpad & WPAD_BUTTON_A) || (gpad & PAD_BUTTON_A))       s_buttons_down |= PD_BTN_A;
    if ((wpad & WPAD_BUTTON_B) || (gpad & PAD_BUTTON_B))       s_buttons_down |= PD_BTN_B;
    if ((wpad & WPAD_BUTTON_HOME) || (gpad & PAD_BUTTON_START)) s_buttons_down |= PD_BTN_HOME;
}

u32 pd_bootstrap_buttons_down(void) {
    return s_buttons_down;
}
