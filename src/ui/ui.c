/* [UNVERIFIED - non compile] voir docs/build.md */
#include <stdio.h>
#include <string.h>

#include <gccore.h>

#include "../core/pd_types.h"
#include "ui.h"

#define PD_STEP_MAX      16
#define PD_STEP_LABEL_MAX 32

static char            s_steps[PD_STEP_MAX][PD_STEP_LABEL_MAX];
static pd_step_state_t s_step_states[PD_STEP_MAX];
static int             s_step_count;

void pd_ui_init(void) {
    /* meme sequence d'init video que WiiMedic (motif standard libogc/devkitPPC,
     * pas de logique specifique a un addon ici) */
    void *xfb;
    GXRModeObj *rmode;

    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
                 rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    s_step_count = 0;
}

void pd_ui_clear(void) {
    printf("\x1b[2J\x1b[0;0H");
}

void pd_ui_banner(const char *console_model, const char *system_version) {
    pd_ui_clear();
    printf("========================================\n");
    printf("             PINOUDIAG WII\n");
    printf("========================================\n\n");
    if (console_model)
        printf("Console : %s\n", console_model);
    if (system_version)
        printf("System  : %s\n", system_version);
    printf("\n");
}

void pd_ui_progress_begin(void) {
    s_step_count = 0;
}

void pd_ui_progress_step(const char *label, pd_step_state_t state) {
    int i;

    /* met a jour l'etape si deja connue, sinon l'ajoute */
    for (i = 0; i < s_step_count; i++) {
        if (strncmp(s_steps[i], label, PD_STEP_LABEL_MAX) == 0) {
            s_step_states[i] = state;
            goto redraw;
        }
    }

    if (s_step_count < PD_STEP_MAX) {
        strncpy(s_steps[s_step_count], label, PD_STEP_LABEL_MAX - 1);
        s_steps[s_step_count][PD_STEP_LABEL_MAX - 1] = '\0';
        s_step_states[s_step_count] = state;
        s_step_count++;
    }

redraw:
    for (i = 0; i < s_step_count; i++) {
        const char *marker;
        switch (s_step_states[i]) {
            case PD_STEP_DONE:    marker = "v"; break;
            case PD_STEP_CURRENT: marker = ">"; break;
            case PD_STEP_ERROR:   marker = "x"; break;
            default:              marker = "o"; break;
        }
        printf("%s %s\n", marker, s_steps[i]);
    }
}

void pd_ui_progress_bar(int percent) {
    int filled, i;
    char bar[21];

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    filled = (percent * 20) / 100;
    for (i = 0; i < 20; i++)
        bar[i] = (i < filled) ? '#' : '.';
    bar[20] = '\0';

    printf("[%s] %d %%\n", bar, percent);
}

void pd_ui_progress_end(void) {
    printf("\n========================================\n");
}

void pd_ui_busy(const char *addon_name, const char *message) {
    printf("> %s\n  %s\n", addon_name, message ? message : "Diagnostic en cours...");
}

void pd_ui_error(pd_error_t err, const char *context) {
    printf("\n[ERREUR] %s", pd_error_str(err));
    if (context && context[0])
        printf(" - %s", context);
    printf("\n");
}

void pd_ui_info(const char *msg) {
    printf("%s\n", msg ? msg : "");
}

void pd_ui_safe_to_remove(void) {
    pd_ui_clear();
    printf("========================================\n\n");
    printf("        DIAGNOSTIC TERMINE\n\n");
    printf("                 [ OK ]\n\n");
    printf("Tous les resultats sont sauvegardes.\n\n");
    printf("       VOUS POUVEZ RETIRER\n");
    printf("             LA CLE USB\n\n");
    printf("========================================\n");
}
