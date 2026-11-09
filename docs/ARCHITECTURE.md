# Architecture d'OpenLooping

## Vue générale

OpenLooping est une application Qt Widgets en C++20. Le modèle métier ne dépend
pas de l'interface, ce qui permet de tester la persistance et les transformations
sans lancer de session graphique.

## Composants

### `src/core/model.*`

Types métier : entités, rubriques, associations, liens, cardinalités,
héritages et règles de gestion. Les identifiants internes sont indépendants des
noms conceptuels et logiques.

### `src/core/persistence.*`

Lecture et écriture du format ouvert `.lrea` en JSON. Le format est versionné et
normalise les invariants comme « identifiant implique NOT NULL ».

### `src/core/loopingimporter.*`

Import en lecture seule des archives natives `.loo`/`.lo1`. Les éléments
directement observés dans l'archive sont distingués des reconstructions
spatiales. Le fichier d'origine n'est jamais réécrit.

### `src/core/relationalgenerator.*`

Projection du MCD vers un MLD textuel et génération SQL. Les avertissements
restent visibles lorsqu'une transformation n'est pas encore couverte.

### `src/ui/`

Fenêtre principale, canevas graphique et dialogues d'édition Qt. Les formes
sont recalculées depuis le modèle à chaque modification ; la position métier
reste persistée dans le document.

### `tests/`

Tests Qt du noyau et de l'interface. Ils couvrent notamment les transformations
de cardinalité, la persistance, l'import natif et l'invariant ID/NOT NULL.

## Formats et frontières de confiance

- `.lrea` : format ouvert propre au projet ;
- `.loo`, `.lo1`… : import natif expérimental et non destructif ;
- SQL/MLD : sorties dérivées à vérifier avant utilisation en production ;
- références propriétaires : conservées exclusivement hors dépôt.
