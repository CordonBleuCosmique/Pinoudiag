/*
 * PinouDiag Wii V1 - test.dol (POC4)
 * [UNVERIFIED - non compile, REQUIRES HARDWARE TEST] voir docs/build.md
 *
 * But unique (section 17, POC4 du brief) : demontrer
 *   PinouDiag -> test.dol -> retour PinouDiag
 * avant de risquer le chainload avec WiiMedic. Ce binaire :
 *   1. affiche un message ;
 *   2. ecrit un marqueur sur USB si disponible ;
 *   3. termine en rappelant le chainloader vers PD_LOADER_BOOT_PATH.
 *
 * Comme documente dans docs/return_to_loader.md, il n'y a pas de "retour"
 * au sens d'un appel de fonction qui reviendrait dans PinouDiag : ce
 * binaire effectue lui-meme un second chainload (aller simple) vers le
 * binaire PinouDiag d'origine. C'est exactement le mecanisme que le
 * fork WiiMedic (addons/WiiMedic/) reutilisera pour de vrai.
 */
#include <fat.h>
#include <gccore.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>

#include "../../../src/core/addon/dol_loader.h"
#include "../../../src/core/pd_types.h"

static void init_video(void) {
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
}

int main(void) {
    int i;
    bool usb_ok;

    init_video();

    printf("\x1b[2J\x1b[0;0H");
    printf("PinouDiag test.dol\n");
    printf("===================\n\n");
    printf("Charge et execute avec succes.\n");

    /* Remonte USB independamment (ce binaire est un chainload separe,
     * il ne partage aucun etat memoire avec le PinouDiag qui l'a lance -
     * voir docs/return_to_loader.md sur le caractere "aller simple"). */
    usb_ok = fatMountSimple("usb", &__io_usbstorage);
    if (usb_ok) {
        FILE *fp = fopen("usb:/PinouDiag_test_marker.txt", "w");
        if (fp) {
            fprintf(fp, "test.dol execute avec succes.\n");
            fflush(fp);
            fclose(fp);
            printf("Marqueur ecrit sur USB:/PinouDiag_test_marker.txt\n");
        } else {
            printf("Echec ecriture marqueur USB (non bloquant)\n");
        }
    } else {
        printf("USB non detectee (non bloquant pour ce test)\n");
    }

    printf("\nRetour vers PinouDiag (%s) dans 3 secondes...\n", PD_LOADER_BOOT_PATH);

    /* Pause visible avant le chainload retour - 3s a 60Hz NTSC / 50Hz PAL,
     * approximatif, suffisant pour la lisibilite du test. */
    for (i = 0; i < 180; i++)
        VIDEO_WaitVSync();

    if (pd_dol_chainload(PD_LOADER_BOOT_PATH) != PD_OK) {
        printf("\n[ERREUR] Impossible de charger %s\n", PD_LOADER_BOOT_PATH);
        printf("Verifier que le fichier existe a la racine de la carte SD.\n");
        for (;;)
            VIDEO_WaitVSync();
    }

    return 0; /* jamais atteint si pd_dol_chainload reussit */
}
