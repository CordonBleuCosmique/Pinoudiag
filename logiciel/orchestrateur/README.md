# pinouNDSdiag — orchestrateur

C'est le programme à utiliser en pratique : **un seul `.nds`** qui enchaîne
automatiquement toutes les étapes de diagnostic, et ne s'arrête que quand une
action humaine est réellement nécessaire (appuyer sur un bouton précis,
toucher l'écran) — le logiciel ne peut pas vérifier ça tout seul.

`affichage-infos/` et `boutons-tactile/` restent disponibles à côté comme
modules indépendants (utile pour retester un point précis rapidement), mais
ce dossier est la version "à lancer et laisser tourner".

## Déroulé automatique

1. **Infos console** (auto) : modèle, batterie, langue, pseudo firmware,
   confirmation visuelle "AFFICHAGE OK" (A pour continuer)
2. **Boutons + croix directionnelle** (semi-auto) : coche automatiquement
   chaque bouton dès qu'il est pressé, avance seule quand tout est testé.
   Maintenir SELECT ~1,5s ignore les boutons restants (bouton cassé/absent,
   pour ne pas bloquer le diagnostic).
3. **Écran tactile** (semi-auto) : avance dès qu'une touche est détectée.
4. **Audio** (semi-auto, avec mesure en direct) : joue un son de test à
   1000 Hz à fond sur les haut-parleurs et l'écoute en même temps via le
   micro (bouclage acoustique), avec un niveau et une fréquence affichés en
   direct à l'écran — voir "Visualiseur audio" ci-dessous. Puis demande de
   brancher un casque et de valider à l'oreille (le micro ne peut pas
   entendre ce qui sort dans un casque).
5. **Résumé + rapport** (auto) : écrit un fichier JSON sur la carte SD, puis
   repropose immédiatement de tester une autre console (appui sur A).

## Visualiseur audio

Pendant le test des haut-parleurs, l'écran affiche en direct :

```
Capte par le micro :
############        (2340)
Frequence : 998 Hz
```

- **Niveau** : une barre + une valeur numérique, calculée à partir de
  l'amplitude (RMS) du signal capté par le micro pendant que le son de test
  joue.
- **Fréquence** : estimée par comptage des passages du signal autour de sa
  moyenne (~998 Hz attendu pour un son de test à 1000 Hz émis correctement).

⚠️ **Ce n'est pas une mesure calibrée.** La DS n'offre aucun moyen de
mesurer un niveau sonore absolu (pas de dB SPL) : la valeur affichée dépend
de la sensibilité du micro, de sa distance aux haut-parleurs, et varie d'un
exemplaire de console à l'autre. Elle sert à deux choses :
- repérer un écart flagrant (haut-parleur très faible, cassé, ou fréquence
  très éloignée de 1000 Hz → signe clair d'un problème) ;
- se constituer, à l'usage, une plage de valeurs "normales" en comparant les
  rapports JSON de plusieurs consoles saines (le niveau crête mesuré est
  enregistré dans chaque rapport, voir ci-dessous).

Le test du casque, lui, reste une validation purement à l'oreille (bouton
A/B/X) : rien sur la DS ne permet de savoir électroniquement ce qui sort
dans un casque branché.

## Identifiant "ticket"

Il n'existe pas d'API fiable pour lire le vrai numéro de série de la
console depuis un homebrew. Le champ `ticket` est donc un **compteur
auto-incrémenté**, stocké dans `pinouNDSdiag_ticket.txt` à la racine de la
carte SD, et non un identifiant matériel. C'est suffisant pour distinguer
chaque passage de test de façon unique et automatique ; à toi d'associer le
numéro de ticket affiché à l'écran à la console physique (post-it, étiquette...)
si tu en as besoin.

## Rapport

Écrit sur la carte SD à :

```
resultat/<ticket>/<horodatage>/resultat.json
```

Exemple de contenu :

```json
{
  "ticket": 7,
  "date": "2026-09-20T11:45:00",
  "modele": "Nintendo DSi",
  "batterie": "12/15",
  "langue": "Francais",
  "pseudo": "",
  "boutons": { "testes": 12, "total": 12, "detail": { "A": true, "...": true } },
  "tactile": { "teste": true, "x": 128, "y": 96 },
  "audio": {
    "haut_parleurs": {
      "teste": true,
      "resultat": "ok",
      "niveau_capte_pic": 2340,
      "note_niveau": "valeur relative non calibree, comparer entre tickets",
      "frequence_mesuree_hz": 998
    },
    "casque": { "teste": true, "resultat": "ok" }
  }
}
```

Si la carte SD n'est pas accessible (pas de pilote DLDI reconnu), le
programme continue quand même et affiche simplement que le rapport n'a pas
pu être enregistré — ça ne bloque jamais le diagnostic en cours.

## Origine du moteur boutons/tactile

La lecture des touches et du tactile (étapes 2 et 3) est adaptée du fork
**Input Test DS** (cphx, domaine public) présent dans `../boutons-tactile/`.
Contrairement au fork gardé tel quel, la logique a été restructurée en liste
à cocher avec une sortie définie (nécessaire pour un enchaînement
automatique) au lieu d'une démo libre sans fin — voir
`../boutons-tactile/PROVENANCE.md` pour la source d'origine intacte.

## Origine du moteur audio

Le test audio (étape 4) est écrit spécifiquement pour Pinoudiag, mais
s'appuie sur l'API micro de libnds (`soundMicRecord`/`soundPlaySample`)
selon le modèle de l'exemple officiel devkitPro
`audio/micrecord` du dépôt `devkitPro/nds-examples` (vérifié pendant le
développement pour la forme exacte du buffer double et du callback micro).

## Prérequis et compilation

Identique aux autres modules — voir `../README.md`.

```sh
cd logiciel/orchestrateur
make
```

Génère `pinouNDSdiag-orchestrateur.nds`.

## Prochaines étapes

- Ajouter les étapes WiFi / rétroéclairage (mires de couleur) quand elles seront écrites
- Rendre le seuil de "SELECT maintenu" configurable si 1,5s s'avère pas assez / trop
- Collecter des `niveau_capte_pic` de plusieurs consoles saines pour définir
  une plage "normale" indicative (voir "Visualiseur audio" ci-dessus)
