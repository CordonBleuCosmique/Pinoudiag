/*---------------------------------------------------------------------------
	pinouNDSdiag - orchestrateur

	Enchaine automatiquement les tests de diagnostic Nintendo DS et ne
	s'arrete que pour les etapes qui exigent une action humaine (appuyer
	sur un bouton, toucher l'ecran) : le logiciel ne peut pas le faire a
	sa place. A la fin de chaque passage, ecrit un rapport JSON sur la
	carte SD :

		resultat/<ticket>/<date>/resultat.json

	<ticket> est un numero auto-incremente (pas un vrai numero de serie
	materiel : aucune API ne l'expose de facon fiable au logiciel), ce qui
	permet quand meme de tracer chaque passage de facon unique et automatique.

	Le moteur de lecture boutons/croix/tactile de l'etape 2 et 3 est adapte
	du fork "Input Test DS" (cphx, domaine public) present dans
	../boutons-tactile/ : memes principes de lecture (scanKeys/touchRead),
	mais restructure en liste a cocher avec sortie definie, puisqu'un
	orchestrateur automatique a besoin d'un etat "termine" - contrairement
	a la demo libre d'origine (voir ../boutons-tactile/PROVENANCE.md).
---------------------------------------------------------------------------*/
#include <nds.h>
#include <maxmod9.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "logo.h"
#include "soundbank.h"
#include "soundbank_bin.h"

/* --------------------------------------------------------------------- */
/* Utilitaires                                                            */
/* --------------------------------------------------------------------- */

static void attendreValidation(const char *message) {
	iprintf("\n\t%s\n\t(A pour continuer)\n", message);
	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_A) {
			break;
		}
	}
}

static int lireEtIncrementerCompteur(const char *chemin) {
	int n = 0;
	FILE *f = fopen(chemin, "r");
	if (f) {
		if (fscanf(f, "%d", &n) != 1) {
			n = 0;
		}
		fclose(f);
	}
	n++;
	f = fopen(chemin, "w");
	if (f) {
		fprintf(f, "%d", n);
		fclose(f);
	}
	return n;
}

/* --------------------------------------------------------------------- */
/* Etape 1 : infos console (100% automatique)                            */
/* --------------------------------------------------------------------- */

typedef struct {
	char modele[24];
	char batterie[16];
	char langue[16];
	char pseudo[16];
} InfosConsole;

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

static void etapeInfosConsole(InfosConsole *infos) {
	iprintf("\x1b[2J");
	iprintf("== Etape 1/3 : Infos console ==\n\n");

	if (isDSiMode()) {
		strcpy(infos->modele, "Nintendo DSi");
	} else {
		strcpy(infos->modele, "Nintendo DS / DS Lite");
	}
	iprintf("Modele    : %s\n", infos->modele);

	unsigned etatBatterie = getBatteryLevel();
	unsigned niveauBatterie = PM_BATT_LEVEL(etatBatterie);
	snprintf(infos->batterie, sizeof(infos->batterie), "%u/15%s",
		niveauBatterie, (etatBatterie & PM_BATT_CHARGING) ? " (charge)" : "");
	iprintf("Batterie  : %s\n", infos->batterie);

	strcpy(infos->langue, nomLangue(PersonalData->language));
	iprintf("Langue    : %s\n", infos->langue);

	infos->pseudo[0] = '\0';
	unsigned longueurPseudo = PersonalData->nameLen;
	if (longueurPseudo > 0 && longueurPseudo <= 10) {
		unsigned i;
		for (i = 0; i < longueurPseudo; i++) {
			s16 c = PersonalData->name[i];
			infos->pseudo[i] = (c >= 32 && c < 127) ? (char)c : '?';
		}
		infos->pseudo[longueurPseudo] = '\0';
	}
	if (infos->pseudo[0]) {
		iprintf("Pseudo    : %s\n", infos->pseudo);
	}

	iprintf("\nAFFICHAGE OK\n");
	attendreValidation("Ecran lisible ?");
}

/* --------------------------------------------------------------------- */
/* Etape 2 : boutons + croix directionnelle                               */
/* --------------------------------------------------------------------- */

#define NB_BOUTONS 12

static const int TOUCHES[NB_BOUTONS] = {
	KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L, KEY_R,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_START, KEY_SELECT
};
static const char *NOMS_BOUTONS[NB_BOUTONS] = {
	"A", "B", "X", "Y", "L", "R",
	"Haut", "Bas", "Gauche", "Droite", "Start", "Select"
};

typedef struct {
	int total;
	int testes;
	int ignores;
} ResultatBoutons;

static void etapeBoutons(ResultatBoutons *resultat, bool teste[NB_BOUTONS]) {
	int i;
	int selectMaintenuFrames = 0;

	for (i = 0; i < NB_BOUTONS; i++) {
		teste[i] = false;
	}

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		int held = keysHeld();

		iprintf("\x1b[2J");
		iprintf("== Etape 2/3 : Boutons ==\n\n");
		iprintf("Appuie sur chaque bouton\nrestant :\n\n");

		int restants = 0;
		for (i = 0; i < NB_BOUTONS; i++) {
			if (held & TOUCHES[i]) {
				teste[i] = true;
			}
			if (!teste[i]) {
				iprintf(" [ ] %s\n", NOMS_BOUTONS[i]);
				restants++;
			}
		}
		if (restants == 0) {
			break;
		}

		iprintf("\n(maintiens SELECT ~1.5s\npour ignorer le reste)\n");

		if (held & KEY_SELECT) {
			selectMaintenuFrames++;
			if (selectMaintenuFrames > 90) {
				break;
			}
		} else {
			selectMaintenuFrames = 0;
		}
	}

	resultat->total = NB_BOUTONS;
	resultat->testes = 0;
	resultat->ignores = 0;
	for (i = 0; i < NB_BOUTONS; i++) {
		if (teste[i]) {
			resultat->testes++;
		} else {
			resultat->ignores++;
		}
	}

	iprintf("\x1b[2J");
	iprintf("== Etape 2/3 : Boutons ==\n\n");
	iprintf("%d/%d boutons testes OK\n", resultat->testes, resultat->total);
	if (resultat->ignores > 0) {
		iprintf("%d ignores manuellement\n", resultat->ignores);
	}
	attendreValidation("Continuer");
}

/* --------------------------------------------------------------------- */
/* Etape 3 : ecran tactile                                                */
/* --------------------------------------------------------------------- */

typedef struct {
	bool teste;
	int dernierX;
	int dernierY;
} ResultatTactile;

static void etapeTactile(ResultatTactile *resultat) {
	touchPosition touch;
	resultat->teste = false;
	resultat->dernierX = -1;
	resultat->dernierY = -1;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		touchRead(&touch);
		int held = keysHeld();

		iprintf("\x1b[2J");
		iprintf("== Etape 3/3 : Tactile ==\n\n");
		iprintf("Touche l'ecran une fois\n\n");

		if (held & KEY_TOUCH) {
			resultat->teste = true;
			resultat->dernierX = touch.px;
			resultat->dernierY = touch.py;
		}

		if (resultat->teste) {
			iprintf("Touche detectee\nX=%d Y=%d\n", resultat->dernierX, resultat->dernierY);
			break;
		}
	}

	attendreValidation(resultat->teste ? "Tactile OK" : "Tactile non teste");
}

/* --------------------------------------------------------------------- */
/* Rapport JSON sur la carte SD                                           */
/* --------------------------------------------------------------------- */

static void ecrireRapport(int ticket, const InfosConsole *infos,
	const ResultatBoutons *boutons, const bool testeBoutons[NB_BOUTONS],
	const ResultatTactile *tactile) {

	if (!fatInitDefault()) {
		iprintf("\nCarte SD non accessible,\nrapport non enregistre.\n");
		attendreValidation("Continuer");
		return;
	}

	time_t maintenant = time(NULL);
	struct tm *t = localtime(&maintenant);

	char dossierTicket[32];
	char dossierDate[64];
	char cheminFichier[96];
	char dateISO[24];

	snprintf(dossierTicket, sizeof(dossierTicket), "resultat/%04d", ticket);
	if (t) {
		snprintf(dossierDate, sizeof(dossierDate), "%s/%04d%02d%02d-%02d%02d%02d",
			dossierTicket, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		snprintf(dateISO, sizeof(dateISO), "%04d-%02d-%02dT%02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
	} else {
		snprintf(dossierDate, sizeof(dossierDate), "%s/inconnue", dossierTicket);
		strcpy(dateISO, "inconnue");
	}
	snprintf(cheminFichier, sizeof(cheminFichier), "%s/resultat.json", dossierDate);

	mkdir("resultat", 0777);
	mkdir(dossierTicket, 0777);
	mkdir(dossierDate, 0777);

	FILE *f = fopen(cheminFichier, "w");
	if (!f) {
		iprintf("\nEchec ecriture rapport :\n%s\n", cheminFichier);
		attendreValidation("Continuer");
		return;
	}

	fprintf(f, "{\n");
	fprintf(f, "  \"ticket\": %d,\n", ticket);
	fprintf(f, "  \"date\": \"%s\",\n", dateISO);
	fprintf(f, "  \"modele\": \"%s\",\n", infos->modele);
	fprintf(f, "  \"batterie\": \"%s\",\n", infos->batterie);
	fprintf(f, "  \"langue\": \"%s\",\n", infos->langue);
	fprintf(f, "  \"pseudo\": \"%s\",\n", infos->pseudo);
	fprintf(f, "  \"boutons\": {\n");
	fprintf(f, "    \"testes\": %d,\n", boutons->testes);
	fprintf(f, "    \"total\": %d,\n", boutons->total);
	fprintf(f, "    \"detail\": {\n");
	for (int i = 0; i < NB_BOUTONS; i++) {
		fprintf(f, "      \"%s\": %s%s\n", NOMS_BOUTONS[i],
			testeBoutons[i] ? "true" : "false",
			(i < NB_BOUTONS - 1) ? "," : "");
	}
	fprintf(f, "    }\n");
	fprintf(f, "  },\n");
	fprintf(f, "  \"tactile\": {\n");
	fprintf(f, "    \"teste\": %s,\n", tactile->teste ? "true" : "false");
	fprintf(f, "    \"x\": %d,\n", tactile->dernierX);
	fprintf(f, "    \"y\": %d\n", tactile->dernierY);
	fprintf(f, "  }\n");
	fprintf(f, "}\n");

	fclose(f);

	iprintf("\nRapport enregistre :\n%s\n", cheminFichier);
}

/* --------------------------------------------------------------------- */
/* main                                                                   */
/* --------------------------------------------------------------------- */

int main(void) {
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_1_2D);
	vramSetBankA(VRAM_A_MAIN_BG);

	consoleDemoInit();

	mmInitDefaultMem((mm_addr)soundbank_bin);
	mmLoad(MOD_XENON);
	mmStart(MOD_XENON, MM_PLAY_LOOP);

	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
	decompress(logoBitmap, BG_GFX, LZ77Vram);

	while (pmMainLoop()) {
		iprintf("\x1b[2J");
		iprintf("   PINOUNDSDIAG\n");
		iprintf("   Orchestrateur\n\n");
		attendreValidation("Demarrer un diagnostic");

		int ticket = lireEtIncrementerCompteur("pinouNDSdiag_ticket.txt");

		InfosConsole infos;
		etapeInfosConsole(&infos);

		ResultatBoutons resultatBoutons;
		bool testeBoutons[NB_BOUTONS];
		etapeBoutons(&resultatBoutons, testeBoutons);

		ResultatTactile resultatTactile;
		etapeTactile(&resultatTactile);

		iprintf("\x1b[2J");
		iprintf("== Resume (ticket %04d) ==\n\n", ticket);
		iprintf("Modele    : %s\n", infos.modele);
		iprintf("Batterie  : %s\n", infos.batterie);
		iprintf("Boutons   : %d/%d\n", resultatBoutons.testes, resultatBoutons.total);
		iprintf("Tactile   : %s\n", resultatTactile.teste ? "OK" : "non teste");

		ecrireRapport(ticket, &infos, &resultatBoutons, testeBoutons, &resultatTactile);

		iprintf("\n(A) console suivante\n");
	}

	return 0;
}
