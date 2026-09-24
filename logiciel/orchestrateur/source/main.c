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

	Le moteur de lecture boutons de l'etape 2 est adapte du fork "Input
	Test DS" (cphx, domaine public) present dans ../boutons-tactile/ :
	memes principes de lecture (scanKeys), mais restructure en liste a
	cocher avec sortie definie, puisqu'un orchestrateur automatique a
	besoin d'un etat "termine" - contrairement a la demo libre d'origine
	(voir ../boutons-tactile/PROVENANCE.md).

	L'etape 3 (tactile) est un test de couverture par peinture, ecrit
	specifiquement pour Pinoudiag (plus rien a voir avec le fork) : le
	stylet peint le pixel exact sous la pointe (largeur fixe 1px, pas de
	pinceau epais qui fausserait la couverture) sur une grille de
	TACTILE_COLONNES x TACTILE_LIGNES zones. Une bande reservee en haut du
	canvas (flèche + palette) permet de changer la couleur sans toucher a
	la largeur. Un saut de position suspect entre deux echantillons
	consecutifs (incoherent avec un geste continu) marque la zone comme
	"glitch" plutot que simplement "ok". L'ecran tactile (toujours
	physiquement en bas) est obtenu en basculant le moteur principal
	bitmap dessus via lcdMainOnBottom() (meme technique que l'etape 5),
	pendant que la console texte (instructions + couverture en direct)
	reste lisible sur l'ecran du haut. Fin de test libre (A a tout moment)
	: les zones jamais peintes restent "non_teste", jamais "probleme".

	L'etape 4 (audio) joue un son de test connu et l'ecoute en meme temps
	via le micro (bouclage acoustique haut-parleurs -> micro), pour
	afficher un niveau et une frequence mesures en direct plutot que de
	se fier uniquement a l'oreille du technicien. Ce n'est PAS une mesure
	calibree (pas de dB SPL absolu : rien sur la DS ne permet de calibrer
	ca) - c'est une valeur relative, utile pour comparer plusieurs
	consoles entre elles au fil du temps (elle est aussi ecrite dans le
	rapport JSON). Le casque ne peut pas etre mesure de cette facon (le
	micro n'entend pas ce qui sort dans les ecouteurs) : validation a
	l'oreille uniquement pour cette partie-la.

	L'etape 5 (ecran) detecte les pixels morts/bloques : remplit chaque
	ecran d'une couleur unie (rouge/vert/bleu/blanc/noir), le technicien
	regarde et signale un defaut (bouton B) ou confirme (A). Detection
	visuelle humaine obligatoire - aucun capteur ne permet au logiciel de
	verifier sa propre dalle. Utilise lcdMainOnTop()/lcdMainOnBottom()
	pour afficher la meme surface pleine-couleur sur l'ecran du haut puis
	celui du bas, sans dupliquer le code d'affichage.

	L'etape 6 (charniere) verifie le capteur de fermeture (KEY_LID) : on
	demande de fermer puis rouvrir le capot et on detecte tout changement
	par rapport a l'etat de depart, sans supposer quelle valeur du bit
	correspond a "ouvert" ou "ferme" (cette polarite n'est pas garantie
	documentee de facon fiable, autant ne pas en dependre). Important :
	pmMainLoop() met normalement la console en veille automatiquement des
	que le capot se ferme (comportement pense pour les jeux), ce qui
	figerait tout le programme au pire moment - d'ou pmSetSleepAllowed(false)
	en tout debut de main().

	Principe "skip != defaut" (applique aux etapes boutons, tactile,
	casque et charniere) : quand une etape est ignoree (accessoire absent,
	SELECT maintenu, timeout...), le rapport JSON ecrit "non_teste" et non
	"probleme" - ne jamais laisser croire a un defaut confirme qui n'a en
	realite pas pu etre observe.
---------------------------------------------------------------------------*/
#include <nds.h>
#include <maxmod9.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
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
	iprintf("== Etape 1/6 : Infos console ==\n\n");

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
		iprintf("== Etape 2/6 : Boutons ==\n\n");
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
	iprintf("== Etape 2/6 : Boutons ==\n\n");
	iprintf("%d/%d boutons testes OK\n", resultat->testes, resultat->total);
	if (resultat->ignores > 0) {
		iprintf("%d ignores manuellement\n", resultat->ignores);
	}
	attendreValidation("Continuer");
}

/* --------------------------------------------------------------------- */
/* Etape 3 : ecran tactile - peinture 1px + couverture par zone           */
/*                                                                         */
/* Le stylet peint le pixel exact sous la pointe (largeur fixe 1px, pas   */
/* de pinceau epais qui fausserait la couverture). L'ecran tactile (bas)  */
/* est bascule sur le moteur principal bitmap via lcdMainOnBottom() -     */
/* meme technique que l'etape 5 - pendant que les instructions/la         */
/* couverture en direct restent lisibles sur l'ecran du haut (console).   */
/* Une bande reservee en haut du canvas (flèche + palette) permet de      */
/* changer la couleur de peinture sans toucher a la largeur.              */
/* --------------------------------------------------------------------- */

#define TACTILE_COLONNES     10
#define TACTILE_LIGNES       8
#define TACTILE_NB_ZONES     (TACTILE_COLONNES * TACTILE_LIGNES)
#define TACTILE_UI_HAUTEUR   20   /* bande du haut du canvas : flèche + palette */
#define TACTILE_SAUT_SUSPECT 40   /* px entre 2 echantillons consecutifs -> glitch */

#define TACTILE_NB_COULEURS 6

typedef struct {
	bool zoneCouverte[TACTILE_NB_ZONES];
	bool zoneGlitch[TACTILE_NB_ZONES];
	int zonesCouvertes;
} ResultatTactile;

static const u16 TACTILE_PALETTE[TACTILE_NB_COULEURS] = {
	(u16)(RGB15(31, 0, 0) | BIT(15)),   /* rouge */
	(u16)(RGB15(0, 31, 0) | BIT(15)),   /* vert */
	(u16)(RGB15(0, 0, 31) | BIT(15)),   /* bleu */
	(u16)(RGB15(31, 31, 0) | BIT(15)),  /* jaune */
	(u16)(RGB15(31, 31, 31) | BIT(15)), /* blanc */
	(u16)(RGB15(0, 31, 31) | BIT(15)),  /* cyan */
};

static void tactileDessinerBande(bool paletteOuverte, int couleurActuelle) {
	/* Bouton fleche : carre 20x20 en haut a gauche */
	for (int y = 0; y < TACTILE_UI_HAUTEUR; y++) {
		for (int x = 0; x < 20; x++) {
			BG_GFX[y * 256 + x] = (u16)(RGB15(15, 15, 15) | BIT(15));
		}
	}
	if (!paletteOuverte) {
		return;
	}
	/* Palette : une pastille par couleur, juste a droite de la fleche */
	for (int i = 0; i < TACTILE_NB_COULEURS; i++) {
		int xDebut = 24 + i * 22;
		for (int y = 0; y < TACTILE_UI_HAUTEUR; y++) {
			for (int x = xDebut; x < xDebut + 20 && x < 256; x++) {
				u16 couleur = TACTILE_PALETTE[i];
				/* Pastille active entouree d'une bordure sombre */
				bool bord = (y == 0 || y == TACTILE_UI_HAUTEUR - 1 ||
					x == xDebut || x == xDebut + 19);
				if (i == couleurActuelle && bord) {
					couleur = (u16)(RGB15(0, 0, 0) | BIT(15));
				}
				BG_GFX[y * 256 + x] = couleur;
			}
		}
	}
}

static void etapeTactile(ResultatTactile *resultat) {
	touchPosition touch;
	for (int i = 0; i < TACTILE_NB_ZONES; i++) {
		resultat->zoneCouverte[i] = false;
		resultat->zoneGlitch[i] = false;
	}
	resultat->zonesCouvertes = 0;

	lcdMainOnBottom();

	/* Fond du canvas en gris fonce pour bien voir les traits peints */
	u16 fond = (u16)(RGB15(6, 6, 8) | BIT(15));
	for (int i = 0; i < 256 * 192; i++) {
		BG_GFX[i] = fond;
	}

	bool paletteOuverte = false;
	int couleurActuelle = 4; /* blanc par defaut */
	tactileDessinerBande(paletteOuverte, couleurActuelle);

	bool toucheAvant = false;
	int xAvant = 0, yAvant = 0;
	bool boutonFlecheAppuyeAvant = false;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		touchRead(&touch);
		int held = keysHeld();

		iprintf("\x1b[2J");
		iprintf("== Etape 3/6 : Tactile ==\n\n");
		iprintf("Balaie tout l'ecran\ntactile avec le\nstylet.\n\n");
		iprintf("Fleche = choisir la\ncouleur (largeur\ntoujours 1px).\n\n");
		iprintf("Couverture : %d%%\n", (resultat->zonesCouvertes * 100) / TACTILE_NB_ZONES);
		iprintf("\n(A) Terminer\n");

		if (held & KEY_TOUCH) {
			int x = touch.px;
			int y = touch.py;

			bool dansBoutonFleche = (y < TACTILE_UI_HAUTEUR && x < 20);
			bool dansPalette = (y < TACTILE_UI_HAUTEUR && x >= 24 &&
				x < 24 + TACTILE_NB_COULEURS * 22);

			if (dansBoutonFleche) {
				if (!boutonFlecheAppuyeAvant) {
					paletteOuverte = !paletteOuverte;
					tactileDessinerBande(paletteOuverte, couleurActuelle);
				}
				boutonFlecheAppuyeAvant = true;
				toucheAvant = false; /* pas de suivi de trait dans la bande */
			} else if (dansPalette && paletteOuverte) {
				int indexCouleur = (x - 24) / 22;
				if (indexCouleur >= 0 && indexCouleur < TACTILE_NB_COULEURS) {
					couleurActuelle = indexCouleur;
					tactileDessinerBande(paletteOuverte, couleurActuelle);
				}
				boutonFlecheAppuyeAvant = false;
				toucheAvant = false;
			} else if (y >= TACTILE_UI_HAUTEUR) {
				boutonFlecheAppuyeAvant = false;

				/* Detection de saut suspect entre 2 echantillons consecutifs */
				int zoneX = (x * TACTILE_COLONNES) / 256;
				int zoneY = ((y - TACTILE_UI_HAUTEUR) * TACTILE_LIGNES) /
					(192 - TACTILE_UI_HAUTEUR);
				if (zoneX >= TACTILE_COLONNES) zoneX = TACTILE_COLONNES - 1;
				if (zoneY >= TACTILE_LIGNES) zoneY = TACTILE_LIGNES - 1;
				int zone = zoneY * TACTILE_COLONNES + zoneX;

				if (toucheAvant) {
					int dx = x - xAvant;
					int dy = y - yAvant;
					int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
					if (distance > TACTILE_SAUT_SUSPECT) {
						resultat->zoneGlitch[zone] = true;
					}
				}

				if (!resultat->zoneCouverte[zone]) {
					resultat->zoneCouverte[zone] = true;
					resultat->zonesCouvertes++;
				}

				u16 couleurTrait = resultat->zoneGlitch[zone] ?
					(u16)(RGB15(31, 0, 0) | BIT(15)) : TACTILE_PALETTE[couleurActuelle];
				BG_GFX[y * 256 + x] = couleurTrait;

				toucheAvant = true;
				xAvant = x;
				yAvant = y;
			}
		} else {
			toucheAvant = false;
			boutonFlecheAppuyeAvant = false;
		}

		if (keysDown() & KEY_A) {
			break;
		}
	}

	lcdMainOnTop();
	decompress(logoBitmap, BG_GFX, LZ77Vram);

	iprintf("\x1b[2J");
	iprintf("== Etape 3/6 : Tactile ==\n\n");
	iprintf("Couverture finale : %d%%\n", (resultat->zonesCouvertes * 100) / TACTILE_NB_ZONES);
	int nbGlitch = 0;
	for (int i = 0; i < TACTILE_NB_ZONES; i++) {
		if (resultat->zoneGlitch[i]) nbGlitch++;
	}
	if (nbGlitch > 0) {
		iprintf("%d zone(s) avec saut\nsuspect (glitch)\n", nbGlitch);
	}
	attendreValidation("Continuer");
}

/* --------------------------------------------------------------------- */
/* Etape 4 : audio (haut-parleurs bouclees sur le micro + casque)         */
/* --------------------------------------------------------------------- */

#define TON_FREQ_HZ      1000
#define TON_SAMPLE_RATE  16000
#define TON_DUREE_S      2
#define TON_NB_ECH       (TON_SAMPLE_RATE * TON_DUREE_S)
#define TON_PI           3.14159265358979323846f

#define MIC_SAMPLE_RATE     8000
#define MIC_TAMPON_OCTETS   (MIC_SAMPLE_RATE * 2 / 30)  /* double-tampon ARM7, ~2 trames */
#define ANALYSE_NB_ECH      1024                        /* ~128 ms a 8000 Hz */

typedef struct {
	bool testeHautParleurs;
	bool hautParleursOk;
	int niveauCretePercu;     /* proxy relatif, pas une mesure calibree */
	int frequenceMesureeHz;
	bool casqueTeste;
	bool casqueOk;
} ResultatAudio;

static u16 tamponAnalyse[ANALYSE_NB_ECH];
static volatile u32 indexAnalyse = 0;
static volatile bool tamponAnalysePret = false;

static void gestionnaireMicro(void *donnees, int longueur) {
	DC_InvalidateRange(donnees, longueur);
	u16 *src = (u16 *)donnees;
	int nbEch = longueur / (int)sizeof(u16);
	for (int i = 0; i < nbEch && indexAnalyse < ANALYSE_NB_ECH; i++) {
		tamponAnalyse[indexAnalyse++] = src[i];
	}
	if (indexAnalyse >= ANALYSE_NB_ECH) {
		tamponAnalysePret = true;
	}
}

/* Analyse le dernier bloc capte : niveau (RMS autour de la moyenne) et
   frequence estimee par comptage de passages autour de la moyenne. */
static void analyserBloc(int *niveau, int *frequenceHz) {
	long somme = 0;
	for (u32 i = 0; i < ANALYSE_NB_ECH; i++) {
		somme += tamponAnalyse[i];
	}
	int moyenne = (int)(somme / ANALYSE_NB_ECH);

	long sommeCarres = 0;
	int passages = 0;
	bool precedentPositif = (tamponAnalyse[0] >= moyenne);
	for (u32 i = 0; i < ANALYSE_NB_ECH; i++) {
		int ecart = (int)tamponAnalyse[i] - moyenne;
		sommeCarres += (long)ecart * ecart;
		bool positif = (tamponAnalyse[i] >= moyenne);
		if (positif != precedentPositif) {
			passages++;
			precedentPositif = positif;
		}
	}

	*niveau = (int)sqrtf((float)sommeCarres / ANALYSE_NB_ECH);

	float dureeFenetre = (float)ANALYSE_NB_ECH / MIC_SAMPLE_RATE;
	*frequenceHz = (int)((passages / 2.0f) / dureeFenetre);
}

static void etapeAudio(ResultatAudio *resultat) {
	resultat->testeHautParleurs = false;
	resultat->hautParleursOk = false;
	resultat->niveauCretePercu = 0;
	resultat->frequenceMesureeHz = 0;
	resultat->casqueTeste = false;
	resultat->casqueOk = false;

	mmStop();

	s16 *tonBuffer = (s16 *)malloc(TON_NB_ECH * sizeof(s16));
	u16 *tamponMic = (u16 *)malloc(MIC_TAMPON_OCTETS);

	if (!tonBuffer || !tamponMic) {
		iprintf("\x1b[2J");
		iprintf("== Etape 4/6 : Audio ==\n\n");
		iprintf("Memoire insuffisante,\ntest audio saute.\n");
		attendreValidation("Continuer");
		if (tonBuffer) free(tonBuffer);
		if (tamponMic) free(tamponMic);
		return;
	}

	for (int i = 0; i < TON_NB_ECH; i++) {
		float phase = 2.0f * TON_PI * TON_FREQ_HZ * i / TON_SAMPLE_RATE;
		tonBuffer[i] = (s16)(sinf(phase) * 30000.0f);
	}

	soundEnable();
	int canalTon = soundPlaySample(tonBuffer, SoundFormat_16Bit,
		TON_NB_ECH * sizeof(s16), TON_SAMPLE_RATE, 127, 64, true, 0);

	/* --- Haut-parleurs : mesure en direct via bouclage micro --- */
	indexAnalyse = 0;
	tamponAnalysePret = false;
	soundMicRecord(tamponMic, MIC_TAMPON_OCTETS, MicFormat_12Bit, MIC_SAMPLE_RATE, gestionnaireMicro);

	int dernierNiveau = 0;
	int dernierFreq = 0;
	int niveauCrete = 0;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();

		if (tamponAnalysePret) {
			analyserBloc(&dernierNiveau, &dernierFreq);
			if (dernierNiveau > niveauCrete) {
				niveauCrete = dernierNiveau;
			}
			indexAnalyse = 0;
			tamponAnalysePret = false;
		}

		iprintf("\x1b[2J");
		iprintf("== Etape 4/6 : Audio ==\n\n");
		iprintf("Son de test %dHz emis\na fond par les\nhaut-parleurs.\n\n", TON_FREQ_HZ);
		iprintf("Capte par le micro :\n");
		int barres = dernierNiveau / 150;
		if (barres > 20) barres = 20;
		for (int b = 0; b < barres; b++) {
			iprintf("#");
		}
		iprintf(" (%d)\n", dernierNiveau);
		iprintf("Frequence : %d Hz\n\n", dernierFreq);
		iprintf("(A) haut-parleurs OK\n(B) probleme\n");

		int appui = keysDown();
		if (appui & KEY_A) {
			resultat->hautParleursOk = true;
			break;
		}
		if (appui & KEY_B) {
			resultat->hautParleursOk = false;
			break;
		}
	}
	soundMicOff();
	resultat->testeHautParleurs = true;
	resultat->niveauCretePercu = niveauCrete;
	resultat->frequenceMesureeHz = dernierFreq;

	/* --- Casque : pas mesurable par le micro, validation a l'oreille --- */
	iprintf("\x1b[2J");
	iprintf("== Etape 4/6 : Audio ==\n\n");
	iprintf("Branche un casque puis\necoute le son de test.\n\n");
	iprintf("(A) casque OK\n(B) probleme\n(X) pas teste\n");

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		int appui = keysDown();
		if (appui & KEY_A) {
			resultat->casqueTeste = true;
			resultat->casqueOk = true;
			break;
		}
		if (appui & KEY_B) {
			resultat->casqueTeste = true;
			resultat->casqueOk = false;
			break;
		}
		if (appui & KEY_X) {
			resultat->casqueTeste = false;
			break;
		}
	}

	soundKill(canalTon);
	free(tonBuffer);
	free(tamponMic);

	mmStart(MOD_XENON, MM_PLAY_LOOP);
}

/* --------------------------------------------------------------------- */
/* Etape 5 : ecran / pixels morts                                         */
/* --------------------------------------------------------------------- */

#define NB_COULEURS_ECRAN 5

static const char *NOMS_COULEURS_ECRAN[NB_COULEURS_ECRAN] = {
	"Rouge", "Vert", "Bleu", "Blanc", "Noir"
};

static const u16 VALEURS_COULEURS_ECRAN[NB_COULEURS_ECRAN] = {
	(u16)(RGB15(31, 0, 0) | BIT(15)),
	(u16)(RGB15(0, 31, 0) | BIT(15)),
	(u16)(RGB15(0, 0, 31) | BIT(15)),
	(u16)(RGB15(31, 31, 31) | BIT(15)),
	(u16)(RGB15(0, 0, 0) | BIT(15)),
};

typedef struct {
	bool defautHaut[NB_COULEURS_ECRAN];
	bool defautBas[NB_COULEURS_ECRAN];
	int nbDefauts;
} ResultatEcran;

static void remplirEcranPrincipal(u16 couleur) {
	for (int i = 0; i < 256 * 192; i++) {
		BG_GFX[i] = couleur;
	}
}

/* Attend A (RAS) ou B (defaut) ; retourne true si defaut signale. */
static bool attendreVerdictEcran(void) {
	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		int appui = keysDown();
		if (appui & KEY_A) {
			return false;
		}
		if (appui & KEY_B) {
			return true;
		}
	}
	return false;
}

static void etapeEcran(ResultatEcran *resultat) {
	resultat->nbDefauts = 0;
	for (int c = 0; c < NB_COULEURS_ECRAN; c++) {
		resultat->defautHaut[c] = false;
		resultat->defautBas[c] = false;
	}

	for (int c = 0; c < NB_COULEURS_ECRAN; c++) {
		remplirEcranPrincipal(VALEURS_COULEURS_ECRAN[c]);

		/* Ecran du haut : le moteur principal (bitmap) y est par defaut,
		   la console texte (instructions) est donc en bas. */
		lcdMainOnTop();
		iprintf("\x1b[2J");
		iprintf("== Etape 5/6 : Ecran ==\n\n");
		iprintf("Couleur : %s\n", NOMS_COULEURS_ECRAN[c]);
		iprintf("-> Ecran du HAUT\n\n");
		iprintf("Pixel mort/colore\nvisible ?\n\n");
		iprintf("(A) RAS   (B) Defaut\n");
		if (attendreVerdictEcran()) {
			resultat->defautHaut[c] = true;
			resultat->nbDefauts++;
		}

		/* Ecran du bas : on bascule le moteur principal en bas, la
		   console texte (donc ces mêmes instructions) passe en haut. */
		lcdMainOnBottom();
		iprintf("\x1b[2J");
		iprintf("== Etape 5/6 : Ecran ==\n\n");
		iprintf("Couleur : %s\n", NOMS_COULEURS_ECRAN[c]);
		iprintf("-> Ecran du BAS\n\n");
		iprintf("Pixel mort/colore\nvisible ?\n\n");
		iprintf("(A) RAS   (B) Defaut\n");
		if (attendreVerdictEcran()) {
			resultat->defautBas[c] = true;
			resultat->nbDefauts++;
		}
	}

	lcdMainOnTop();
	decompress(logoBitmap, BG_GFX, LZ77Vram);

	iprintf("\x1b[2J");
	iprintf("== Etape 5/6 : Ecran ==\n\n");
	if (resultat->nbDefauts == 0) {
		iprintf("Aucun defaut signale\nsur les %d couleurs.\n", NB_COULEURS_ECRAN);
	} else {
		iprintf("%d defaut(s) signale(s).\n", resultat->nbDefauts);
	}
	attendreValidation("Continuer");
}

/* --------------------------------------------------------------------- */
/* Etape 6 : capteur de fermeture (charniere)                             */
/* --------------------------------------------------------------------- */

#define CHARNIERE_TIMEOUT_FRAMES (60 * 15)  /* ~15s a 60Hz */

typedef struct {
	bool teste;   /* un changement d'etat du capteur a ete detecte */
	bool ignore;  /* teste=false ET ignore=false -> non teste (timeout) */
} ResultatCharniere;

/* Ne suppose pas quelle valeur du bit correspond a "ouvert" ou "ferme" -
   detecte juste un changement par rapport a l'etat de depart, ce qui
   suffit a prouver que le capteur reagit physiquement. */
static void etapeCharniere(ResultatCharniere *resultat) {
	resultat->teste = false;
	resultat->ignore = false;

	scanKeys();
	int etatInitial = keysCurrent() & KEY_LID;
	int selectMaintenuFrames = 0;
	int compteurFrames = 0;

	while (pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();

		iprintf("\x1b[2J");
		iprintf("== Etape 6/6 : Charniere ==\n\n");
		iprintf("Ferme puis rouvre\nle capot.\n\n");
		iprintf("(maintiens SELECT ~1.5s\npour ignorer ce test)\n");

		int etatCourant = keysCurrent() & KEY_LID;
		if (etatCourant != etatInitial) {
			resultat->teste = true;
			break;
		}

		if (keysHeld() & KEY_SELECT) {
			selectMaintenuFrames++;
			if (selectMaintenuFrames > 90) {
				resultat->ignore = true;
				break;
			}
		} else {
			selectMaintenuFrames = 0;
		}

		compteurFrames++;
		if (compteurFrames > CHARNIERE_TIMEOUT_FRAMES) {
			resultat->ignore = true;
			break;
		}
	}

	iprintf("\x1b[2J");
	iprintf("== Etape 6/6 : Charniere ==\n\n");
	if (resultat->teste) {
		iprintf("Changement detecte :\ncapteur OK\n");
	} else {
		iprintf("Aucun changement\ndetecte (non teste).\n");
	}
	attendreValidation("Continuer");
}

/* --------------------------------------------------------------------- */
/* Rapport JSON sur la carte SD                                           */
/* --------------------------------------------------------------------- */

static void ecrireRapport(int ticket, const InfosConsole *infos,
	const ResultatBoutons *boutons, const bool testeBoutons[NB_BOUTONS],
	const ResultatTactile *tactile, const ResultatAudio *audio,
	const ResultatEcran *ecran, const ResultatCharniere *charniere) {

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
		/* teste[i]==false signifie toujours "pas encore atteint quand le
		   technicien a arrete le test" (via SELECT), jamais "confirme
		   casse" : la boucle ne s'arrete que si tout est teste ou si
		   le reste est ignore d'un coup. D'ou "non_teste" et pas
		   "probleme" ici, pour ne pas laisser croire a un defaut
		   confirme qui n'a pas ete observe. */
		fprintf(f, "      \"%s\": \"%s\"%s\n", NOMS_BOUTONS[i],
			testeBoutons[i] ? "ok" : "non_teste",
			(i < NB_BOUTONS - 1) ? "," : "");
	}
	fprintf(f, "    }\n");
	fprintf(f, "  },\n");
	fprintf(f, "  \"tactile\": {\n");
	fprintf(f, "    \"couverture_pct\": %d,\n",
		(tactile->zonesCouvertes * 100) / TACTILE_NB_ZONES);
	fprintf(f, "    \"grille\": \"%dx%d\",\n", TACTILE_COLONNES, TACTILE_LIGNES);
	fprintf(f, "    \"detail\": {\n");
	for (int i = 0; i < TACTILE_NB_ZONES; i++) {
		const char *statut = tactile->zoneGlitch[i] ? "glitch" :
			(tactile->zoneCouverte[i] ? "ok" : "non_teste");
		fprintf(f, "      \"zone_%02d\": \"%s\"%s\n", i, statut,
			(i < TACTILE_NB_ZONES - 1) ? "," : "");
	}
	fprintf(f, "    }\n");
	fprintf(f, "  },\n");
	fprintf(f, "  \"audio\": {\n");
	fprintf(f, "    \"haut_parleurs\": {\n");
	fprintf(f, "      \"teste\": %s,\n", audio->testeHautParleurs ? "true" : "false");
	fprintf(f, "      \"resultat\": \"%s\",\n", audio->hautParleursOk ? "ok" : "probleme");
	fprintf(f, "      \"niveau_capte_pic\": %d,\n", audio->niveauCretePercu);
	fprintf(f, "      \"note_niveau\": \"valeur relative non calibree, comparer entre tickets\",\n");
	fprintf(f, "      \"frequence_mesuree_hz\": %d\n", audio->frequenceMesureeHz);
	fprintf(f, "    },\n");
	fprintf(f, "    \"casque\": {\n");
	fprintf(f, "      \"teste\": %s,\n", audio->casqueTeste ? "true" : "false");
	fprintf(f, "      \"resultat\": \"%s\"\n", !audio->casqueTeste ? "non_teste" : (audio->casqueOk ? "ok" : "probleme"));
	fprintf(f, "    }\n");
	fprintf(f, "  },\n");
	fprintf(f, "  \"ecran\": {\n");
	fprintf(f, "    \"nb_defauts\": %d,\n", ecran->nbDefauts);
	fprintf(f, "    \"detail\": {\n");
	for (int i = 0; i < NB_COULEURS_ECRAN; i++) {
		fprintf(f, "      \"%s\": { \"defaut_haut\": %s, \"defaut_bas\": %s }%s\n",
			NOMS_COULEURS_ECRAN[i],
			ecran->defautHaut[i] ? "true" : "false",
			ecran->defautBas[i] ? "true" : "false",
			(i < NB_COULEURS_ECRAN - 1) ? "," : "");
	}
	fprintf(f, "    }\n");
	fprintf(f, "  },\n");
	fprintf(f, "  \"charniere\": {\n");
	fprintf(f, "    \"resultat\": \"%s\"\n", charniere->teste ? "ok" : "non_teste");
	fprintf(f, "  }\n");
	fprintf(f, "}\n");

	fclose(f);

	iprintf("\nRapport enregistre :\n%s\n", cheminFichier);
}

/* --------------------------------------------------------------------- */
/* main                                                                   */
/* --------------------------------------------------------------------- */

int main(void) {
	/* Empeche la mise en veille automatique quand le capot se ferme :
	   pmMainLoop() la declenche par defaut, ce qui figerait tout le
	   programme (utile pour un jeu, pas pour un outil de diagnostic -
	   et carrement genant pour l'etape 6 qui teste justement le capot). */
	pmSetSleepAllowed(false);

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

		ResultatAudio resultatAudio;
		etapeAudio(&resultatAudio);

		ResultatEcran resultatEcran;
		etapeEcran(&resultatEcran);

		ResultatCharniere resultatCharniere;
		etapeCharniere(&resultatCharniere);

		iprintf("\x1b[2J");
		iprintf("== Resume (ticket %04d) ==\n\n", ticket);
		iprintf("Modele    : %s\n", infos.modele);
		iprintf("Batterie  : %s\n", infos.batterie);
		iprintf("Boutons   : %d/%d\n", resultatBoutons.testes, resultatBoutons.total);
		iprintf("Tactile   : %d%% couvert\n",
			(resultatTactile.zonesCouvertes * 100) / TACTILE_NB_ZONES);
		iprintf("HP        : %s (%d Hz)\n",
			resultatAudio.hautParleursOk ? "OK" : "probleme",
			resultatAudio.frequenceMesureeHz);
		iprintf("Casque    : %s\n",
			!resultatAudio.casqueTeste ? "non teste" : (resultatAudio.casqueOk ? "OK" : "probleme"));
		iprintf("Ecran     : %s\n",
			resultatEcran.nbDefauts == 0 ? "OK" : "defauts signales");
		iprintf("Charniere : %s\n", resultatCharniere.teste ? "OK" : "non teste");

		ecrireRapport(ticket, &infos, &resultatBoutons, testeBoutons, &resultatTactile, &resultatAudio, &resultatEcran, &resultatCharniere);

		iprintf("\n(A) console suivante\n");
	}

	return 0;
}
