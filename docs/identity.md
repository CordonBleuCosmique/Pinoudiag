# Identification / ConsoleFingerprint (section 7 du brief)

Consigne du brief : ne pas supposer que "NAND + CPU + GPU" est la meilleure
identité — étudier chaque candidat sur accessibilité, unicité, stabilité,
reproductibilité, confidentialité, comportement après remplacement matériel.

Candidats identifiés dans le code WiiMedic (`system_info.c`, [VERIFIED] —
tous accessibles depuis du homebrew sans droits particuliers) :

| Candidat | Accessibilité | Unicité | Stabilité | Reproductibilité | Confidentialité | Après remplacement matériel |
|---|---|---|---|---|---|---|
| `ES_GetDeviceID()` | Directe, un appel ES | Unique par console (lié à la paire de clés NAND/ECC générée en usine) | Stable tant que la NAND d'origine n'est pas remplacée | Oui, même valeur à chaque boot | Semi-sensible (identifiant système, mais pas une donnée personnelle type MAC) | **Change si la puce NAND/Hollywood est remplacée** (ex. réparation après brick sévère) — cassera la continuité `WII-XXXXXXXX/` |
| Adresse MAC WiFi | Nécessaire de toute façon pour LetterBomb (donnée déjà en possession du technicien) | Unique par carte WiFi | Stable | Oui | **Sensible** — identifiant réseau, à ne jamais exposer en clair dans un nom de dossier public/partageable | Change si le module WiFi est remplacé (rare) |
| `SYS_GetHollywoodRevision()` | Directe | **Pas unique** — juste une révision de silicium (ex. `0x21` = late revision), partagée par des millions de consoles | Stable | Oui | Non sensible | Change seulement si la carte mère entière est remplacée |
| Numéro de série (sérigraphié, pas lisible en logiciel standard) | **Non accessible depuis l'API homebrew observée** dans WiiMedic — aucune fonction ES/SYS ne l'expose dans le code lu | — | — | — | — | — |
| Révision matérielle (RVL-001 vs RVL-101) | Indirecte (pas de fonction dédiée vue dans le code ; déductible via d'autres signaux hardware non explorés ici) | Faible (deux valeurs possibles seulement) | Stable | Oui | Non sensible | Ne change pas (propriété du modèle, pas de la carte mère) |
| `ES_GetBoot2Version()` | Directe | Pas unique (version logicielle boot2) | Peut changer si boot2 est mis à jour/downgradé (rare, risqué) | Oui à un instant donné | Non sensible | N/A |

## Analyse

- **Aucun identifiant seul n'est à la fois unique, stable ET non sensible.**
  Le `device_id` (ES) est le meilleur candidat d'unicité/stabilité mais est
  un identifiant système à traiter comme sensible (ne jamais l'écrire en
  clair comme nom de dossier — cf. section 7 du brief : "ne pas exposer
  inutilement les identifiants bruts").
- La MAC WiFi est déjà collectée par l'utilisateur pour générer la lettre
  LetterBomb (hors périmètre PinouDiag), mais PinouDiag ne doit pas
  supposer qu'il peut la relire facilement/fiablement depuis le code
  (à vérifier : accessible via une fonction réseau libogc, non explorée
  dans WiiMedic qui ne l'utilise que pour l'affichage d'info WiFi générique,
  pas comme identifiant).

## Proposition ConsoleFingerprint (V1, à valider)

```
fingerprint = HASH( ES_DeviceID || SYS_HollywoodRevision || boot2_version )
dossier     = "WII-" + hex(fingerprint)[:8]   // tronqué, pas l'ID brut
```

- Utiliser un hash (ex. SHA-1 déjà présent dans le code HBC cloné,
  `channel/channelapp/source/sha1.c`, réutilisable) plutôt que de
  concaténer les identifiants bruts dans le nom de dossier — répond
  directement à "ne pas exposer inutilement les identifiants bruts".
- Combiner plusieurs signaux plutôt qu'un seul réduit le risque de
  collision fortuite tout en gardant une instabilité limitée (seul
  `device_id` peut changer après réparation NAND — un remplacement complet
  de Hollywood est rare et casse de toute façon la continuité d'identité
  physique de la console).
- [REQUIRES HARDWARE TEST] Cette proposition n'a pas été validée sur
  plusieurs Wii physiques (POC 8 du brief). À tester explicitement : deux
  boots de la même Wii donnent-ils bien le même hash ? Une Wii après
  remplacement NAND en usine partenaire donne-t-elle un hash différent (et
  donc un nouveau dossier `Results/` — comportement attendu et documenté,
  pas un bug) ?

## Statut

[VERIFIED] pour l'accessibilité des champs listés (lecture directe du code
WiiMedic qui les utilise déjà). [UNVERIFIED]/[REQUIRES HARDWARE TEST] pour
la stabilité/unicité réelle du hash combiné proposé — aucune mesure sur
plusieurs consoles n'a été faite dans cette phase de recherche.
