# pinouNDSdiag

Module de diagnostic **Pinoudiag** spécialisé pour la **Nintendo DS** (DS, DS Lite, DSi, DSi XL).

Ce dossier regroupe les fiches de diagnostic, les pannes courantes et les repères de connectique
(pinouts) pour aider à identifier et réparer les problèmes matériels des consoles Nintendo DS.

## Structure

- `ecran/` — pannes d'affichage (écran noir, lignes, rétroéclairage, nappe supérieure/inférieure)
- `boutons/` — boutons et croix directionnelle qui ne répondent pas ou restent bloqués
- `audio/` — absence de son, haut-parleurs, prise casque
- `alimentation/` — ne s'allume pas, batterie, charge, connecteur d'alimentation
- `connectique/` — cartouches (jeu / GBA), fentes, contacts, charnière
- `carte-mere/` — composants, points de test, schémas de la carte mère

## Utilisation

Chaque sous-dossier contiendra, au fur et à mesure :
1. Une liste de symptômes observés
2. Les causes probables classées par fréquence
3. Les étapes de diagnostic (tests multimètre, points de mesure, etc.)
4. Les pinouts / schémas de connecteurs concernés

## À compléter

Ce dossier est une base de départ. Ajoute les fiches de diagnostic au fur et à mesure des cas
rencontrés (modèle de console, symptôme, solution trouvée).
