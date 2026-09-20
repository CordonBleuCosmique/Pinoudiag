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
4. **Résumé + rapport** (auto) : écrit un fichier JSON sur la carte SD, puis
   repropose immédiatement de tester une autre console (appui sur A).

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
  "tactile": { "teste": true, "x": 128, "y": 96 }
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

## Prérequis et compilation

Identique aux autres modules — voir `../README.md`.

```sh
cd logiciel/orchestrateur
make
```

Génère `pinouNDSdiag-orchestrateur.nds`.

## Prochaines étapes

- Ajouter les étapes audio / WiFi / rétroéclairage quand elles seront écrites
- Rendre le seuil de "SELECT maintenu" configurable si 1,5s s'avère pas assez / trop
