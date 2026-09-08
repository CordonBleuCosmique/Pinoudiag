# SD / USB — accès stockage

Source primaire : `source/main.c`, `source/storage_test.c`, `source/report.c`
de WiiMedic ([VERIFIED], lecture directe).

## Montage

- `fatInitDefault()` (libfat), appelé une fois dans `main()` avant toute
  opération fichier. Monte automatiquement tous les devices FAT disponibles
  et enregistre les préfixes `sd:/` et `usb:/` (device names devoptab
  standard libfat/libogc pour SD slot interne et port(s) USB).
- Détection de présence : `opendir("sd:/")` / `opendir("usb:/")` réussit ou
  échoue — pas d'API de détection dédiée séparée du simple accès
  filesystem dans le code observé.

## Écriture

- `fopen(path, "w")` / `fopen(path, "wb")` standard stdio (via devoptab).
  Pas de gestion d'erreur avancée au-delà de vérifier que `fopen` ne
  retourne pas `NULL` (carte pleine, protégée en écriture, etc. → message
  d'erreur générique à l'utilisateur, pas de code d'erreur distingué).
- Le brief (section 5) exige que **PinouDiag lui-même n'écrive jamais sur la
  SD** (SD = lecture seule pendant l'exécution) — cohérent avec le fait que
  WiiMedic écrit son propre rapport en dur sur `sd:` en priorité, puis
  `usb:` si `sd:` indisponible. **Point d'attention direct pour
  l'intégration** : si une carte SD est présente et accessible en écriture,
  WiiMedic y écrira `WiiMedic_Report.txt` **avant** de tenter l'USB, ce qui
  contredit la contrainte "SD en lecture seule" du brief pour PinouDiag
  lui-même (WiiMedic, en tant qu'addon externe non modifié, ne connaît pas
  cette contrainte).
  - Option : forcer WiiMedic à toujours écrire sur USB en rendant la SD
    non language-writable au moment de son exécution n'est pas réaliste
    (la carte physique reste en écriture normale).
  - Option retenue recommandée : **laisser WiiMedic écrire où il veut**
    (SD ou USB selon sa propre logique), puis **PinouDiag copie/déplace**
    le rapport WiiMedic vers `USB:/Results/.../wiimedic.txt` lors de
    l'étape "analyse" (section 10-11 du brief), et supprime la copie
    laissée par WiiMedic sur SD si elle existe (nettoyage post-run,
    cohérent avec "SD = lecture seule" au niveau de l'état final observable
    par l'utilisateur, même si WiiMedic lui-même y a transitoirement
    écrit). À documenter clairement comme comportement voulu, pas un
    oubli.
- Chemins en dur non paramétrables dans WiiMedic (`REPORT_PATH_SD`,
  `REPORT_PATH_USB` = constantes `#define`) — PinouDiag ne peut pas lui
  dire où écrire via un argument ; il doit aller chercher le fichier à un
  chemin connu et fixe après l'exécution de l'addon.

## Lecture

- `readdir`/`stat` standard pour lister fichiers/dossiers (`storage_test.c`,
  comptage fichiers/dossiers racine + présence de `/apps`).

## Benchmark (référence, pas à réimplémenter)

1 Mo, blocs de 32 Ko, 3 itérations moyennées, écrit puis relit un fichier
temporaire `wiimedic_bench.tmp`, supprimé après coup. Seuils : ≥2000 KB/s =
"Excellent", ≥1000 KB/s = "OK", en dessous = "Lent". PinouDiag n'a pas
besoin de refaire ce test — le brief interdit de dupliquer un diagnostic
déjà couvert par l'addon (section 0 : "PinouDiag ne réimplémente pas les
diagnostics qui existent déjà").

## Ce que PinouDiag doit implémenter lui-même (hors WiiMedic)

- Détection USB **avant** de lancer le workflow (section 5-6 du brief :
  écriture des `Results/`, du fichier de session `PinouDiag.session`, etc.
  se fait sur USB, jamais sur SD).
- Gestion explicite de l'absence d'USB (section 14) — WiiMedic, lui, bascule
  silencieusement vers SD si USB absent ; PinouDiag doit avoir sa **propre**
  détection indépendante de celle de WiiMedic puisque ses contraintes
  (SD=lecture seule, résultats persistants sur USB uniquement) sont plus
  strictes que celles de l'addon.
- Flush/fermeture propre du stockage avant `SAFE_TO_REMOVE` (section 15) —
  aucune fonction dédiée observée dans le code WiiMedic pour ça (il ne gère
  pas de séquence d'arrêt propre du filesystem, juste un reset système
  classique) ; à implémenter côté PinouDiag avec les primitives libogc/libfat
  standard (`fatUnmount`/équivalent — [UNVERIFIED, à vérifier dans la doc
  libfat, non auditée dans cette phase]).

## Statut

[VERIFIED] pour tout ce qui décrit le comportement effectif de WiiMedic.
[UNVERIFIED]/[REQUIRES HARDWARE TEST] pour les recommandations
d'implémentation PinouDiag ci-dessus, qui restent à écrire et tester.
