# Première implémentation de l'import natif `.loo` / `.lo1`

## Portée livrée en 0.5.1

L'ouverture reconnaît une archive Looping à partir de son contenu binaire et
non de son extension. Les extensions `.loo`, `.lo1` à `.lo9` sont proposées
dans le dialogue afin de couvrir le document principal et les sauvegardes
numérotées. Après import, le chemin d'enregistrement est volontairement vide :
« Enregistrer » déclenche donc « Enregistrer sous » au format `.lrea` et ne
peut pas écraser l'archive Windows d'origine.

La portée importée sans perte est actuellement :

- document vide ;
- document contenant exactement une entité, sans rubrique, association ou
  lien, avec récupération du nom Unicode et de la position ;
- même traitement pour `.loo` et `.lo1`.

Les archives riches disposent maintenant d'un mode de récupération explicite.
Il restaure les cadres d'entités, leurs positions, noms et rubriques ainsi que
les noms d'associations retrouvés. Un avertissement indique que les liens,
cardinalités, identifiants et propriétés avancées ne sont pas encore décodés.
Cette récupération n'est jamais silencieuse et le fichier source demeure en
lecture seule.

## Extension livrée en 0.6.0

Les micro-modèles contrôlés ont permis d'ajouter :

- le drapeau d'identifiant de chaque rubrique ; un identifiant est normalisé
  en `NOT NULL`, conformément à la sémantique d'une clé primaire ;
- la position native des associations ;
- les cardinalités observées dans `CDrawArc` (`0 = 0,1`, `1 = 1,1`,
  `2 = 0,n`, `3 = 1,n`) ;
- les branches lorsque le nombre d'arcs connectés correspond exactement à
  deux branches par association.

La correspondance entre une branche et son entité est actuellement reconstruite
à partir des deux entités les plus proches, puis stabilisée selon l'ordre des
entités dans l'archive. Cette étape est une inférence explicite et reste annoncée
à l'utilisateur. Les positions, drapeaux d'identifiant et valeurs de cardinalité
sont, eux, directement observés dans l'archive. `UNIQUE` et le `NOT NULL`
indépendant d'un identifiant restent hors de la portée décodée.

## Observations établies

- Les échantillons sont des archives binaires MFC, pas du JSON, XML ou ZIP.
- Le flux d'objets observé commence à `0xA20` et référence notamment
  `CDrawEntite`, `CDrawTexte`, `CDrawArc` et `CDrawRelation`.
- `sample-one-entity.lo1` et `sample-one-entity.loo` ont la même taille et la
  même empreinte SHA-256. Cela établit l'identité de ces deux échantillons,
  mais ne prouve pas que toutes les versions de Looping utilisent toujours
  `.lo1` de cette façon.
- L'analyse statique du binaire 32 bits montre que `CDrawEntite::Serialize`
  appelle d'abord la sérialisation de sa classe de base, puis lit/écrit ses
  propriétés propres. `CDrawTexte::Serialize` utilise le mécanisme Unicode de
  `CArchive`. Les fonctions ont été récupérées par Ghidra ; le pseudocode est
  dérivé et le PDB original n'est pas disponible.

## Tests automatiques

Les tests couvrent l'import du fichier vide, d'une entité en `.loo`, de la
même entité en `.lo1`, la conservation exacte des octets source, ainsi que la
récupération d'un modèle complexe à deux entités, deux rubriques et une
association. Les variantes `0,n–0,1` et `0,n–1,1`, la position des associations,
la normalisation identifiant/`NOT NULL` et la conservation du fichier source
sont désormais testées automatiquement. Le décodage direct des références
d'extrémité des arcs et les variantes historiques restent à compléter.
