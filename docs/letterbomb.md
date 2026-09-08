# LetterBomb

[UNVERIFIED — synthèse WebSearch, wiibrew.org inaccessible directement dans ce
sandbox (EGRESS_BLOCKED). À recroiser avec une lecture directe de
https://wiibrew.org/wiki/LetterBomb et https://wiibrew.org/wiki/LetterBomb/fr
dès que possible.]

## Mécanisme

- Exploite le canal **Wii Message Board** (Wii no Ma) en générant une lettre
  malformée qui, à l'ouverture, fait planter le canal de façon contrôlée pour
  exécuter du code arbitraire.
- Nécessite **System Menu 4.3** exactement (versions antérieures utilisaient
  Bannerbomb à la place, qui ne fonctionne pas sur 4.3).
- Nécessite l'**adresse MAC WiFi** de la console pour générer une lettre
  valide pour cette Wii précise (la lettre est adressée à ce MAC).
- Une fois exécuté, le code charge depuis la **racine de la carte SD** :
  `boot.elf` en priorité, sinon `boot.dol` en repli.

## Ce que ça implique pour PinouDiag

- PinouDiag n'a **rien à faire** pour cette étape : le développement
  commence au moment où LetterBomb a déjà exécuté `boot.elf`/`boot.dol`
  depuis la racine SD. Le binaire PinouDiag doit donc être livré sous
  `SD:/boot.elf` (ou `.dol`) — cohérent avec la structure cible du brief
  (`SD:/PinouDiag/boot.elf` + un `boot.elf` racine qui chainload vers ce
  dossier, ou directement `SD:/boot.elf` = PinouDiag lui-même — à trancher
  en implémentation selon si on veut garder la racine SD "propre").
- Le brief interdit explicitement de réimplémenter LetterBomb, d'automatiser
  l'ouverture de la lettre, ou d'installer HBC — PinouDiag n'a donc aucune
  dépendance de code vis-à-vis de LetterBomb au-delà du point d'entrée
  `boot.elf`/`boot.dol`.
- Contrainte système 4.3 : à confirmer si le System Menu doit *rester* en
  4.3 pendant toute l'exécution de PinouDiag (a priori oui — PinouDiag ne
  touche jamais au System Menu ni à la NAND, donc rien ne devrait le faire
  changer de version pendant le diagnostic).

## Statut

[REQUIRES VERIFICATION] Relecture directe de WiiBrew nécessaire avant de
considérer ce document comme définitif. Aucune décision d'implémentation ne
dépend cependant de ce fichier au-delà du point d'entrée `boot.elf`, ce qui
limite le risque si un détail secondaire s'avérait inexact.
