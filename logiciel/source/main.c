/*---------------------------------------------------------------------------
	pinouNDSdiag - logiciel de test / diagnostic pour Nintendo DS

	Premiere brique : verifie que l'affichage fonctionne ("AFFICHAGE OK")
	et affiche les informations console disponibles (modele, batterie,
	langue et pseudo de la console).
---------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>

static const char *nomLangue(unsigned langue) {
	switch (langue) {
		case 0:  return "Japonais";
		case 1:  return "Anglais";
		case 2:  return "Francais";
		case 3:  return "Allemand";
		case 4:  return "Italien";
		case 5:  return "Espagnol";
		case 6:  return "Chinois";
		default: return "Inconnu";
	}
}

static void afficherInfosConsole(void) {
	iprintf("-- Infos console --\n");

	if (isDSiMode()) {
		iprintf("Modele    : Nintendo DSi\n");
	} else {
		iprintf("Modele    : Nintendo DS / DS Lite\n");
	}

	unsigned etatBatterie = getBatteryLevel();
	unsigned niveauBatterie = PM_BATT_LEVEL(etatBatterie);
	iprintf("Batterie  : %u/15%s\n",
		niveauBatterie,
		(etatBatterie & PM_BATT_CHARGING) ? " (en charge)" : "");

	iprintf("Langue    : %s\n", nomLangue(PersonalData->language));

	unsigned longueurPseudo = PersonalData->nameLen;
	if (longueurPseudo > 0 && longueurPseudo <= 10) {
		char pseudo[11];
		unsigned i;
		for (i = 0; i < longueurPseudo; i++) {
			s16 caractere = PersonalData->name[i];
			pseudo[i] = (caractere >= 32 && caractere < 127) ? (char)caractere : '?';
		}
		pseudo[longueurPseudo] = '\0';
		iprintf("Pseudo    : %s\n", pseudo);
	}
}

int main(void) {
	consoleDemoInit();

	iprintf("========================\n");
	iprintf("     AFFICHAGE OK\n");
	iprintf("========================\n\n");

	afficherInfosConsole();

	iprintf("\nAppuie sur START pour quitter.\n");

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_START) {
			break;
		}
	}

	return 0;
}
