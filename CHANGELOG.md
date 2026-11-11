# Historique des versions

Ce journal combine les sources actuellement disponibles, les rapports d'audit
datés et les paquets Debian conservés. Les versions dont les sources n'ont pas
survécu sont explicitement signalées : aucune source historique n'a été
fabriquée pour remplir les trous.

## Non publié

- Refonte complète de la barre supérieure : accès direct aux outils MCD,
  pictogrammes vectoriels cohérents, groupes allégés, commandes de document
  compactes, zoom modernisé et meilleure hiérarchie visuelle.
- Modes de construction réellement exclusifs : le bouton Lien n’est bleu que
  pendant son utilisation et Sélection reprend l’état actif à l’annulation.
- Réintégration visible des outils MCD avancés et des représentations prévues
  dans un menu dédié, avec indication honnête des fonctions encore à venir.
- Ajout d’un aperçu dynamique pendant la création des liens : extrémité de
  départ encadrée, liaison suivant la souris et cible compatible mise en
  évidence.
- Zone cliquable élargie autour des liens, surlignage bleu à la sélection et
  affichage du rôle à côté de la cardinalité.
- Correction du mode de création des liens sur le canevas.
- Sélection des extrémités dans les deux ordres : entité–association ou
  association–entité, ainsi qu'entité–héritage dans les deux sens.
- Mode lien persistant pour poser plusieurs branches successivement, avec
  curseur dédié, messages d'étape, refus des doublons et annulation par
  `Échap`.
- Test d'interface utilisant de véritables clics de souris sur les objets du
  canevas.

## 0.6.1 — 9 novembre 2026

- Unification complète de l'identité sous le nom OpenLooping : application,
  fenêtre, exécutable, paquet Debian, entrée de bureau, type MIME et format
  JSON ouvert.
- Refonte du confort visuel du canevas MCD.
- Compactage proportionnel des coordonnées importées.
- Dimensionnement des entités selon leurs attributs, avec ellipse et infobulle
  pour les libellés exceptionnellement longs.
- Raccordement des liens au contour exact des rectangles et ellipses.
- Badges de cardinalité, grille discrète, ombres douces et palette harmonisée.
- Ajustement et centrage automatiques du modèle à l'ouverture.
- Ouverture directe d'un modèle depuis la ligne de commande.

## 0.6.0 — 4 novembre 2026

- Import enrichi des archives natives `.loo` et `.lo1`.
- Récupération des positions d'entités et d'associations, rubriques,
  identifiants, branches et cardinalités observées.
- Reconstruction spatiale prudente des extrémités de branches, accompagnée
  d'un avertissement explicite.
- Invariant métier et graphique : un identifiant implique toujours `NOT NULL`.
- Tests automatisés des cardinalités natives `0,1`, `1,1`, `0,n` et `1,n`.

## 0.5.1 — 4 novembre 2026

- Première implémentation documentée de l'import natif Looping.
- Reconnaissance par contenu des archives MFC, indépendamment de l'extension.
- Import sans perte des documents vides et des modèles à une entité.
- Mode de récupération partielle clairement signalé pour les modèles riches.
- Protection systématique du fichier Windows original.

## 0.5.0 — archive binaire conservée

Le paquet Debian a survécu, mais pas un instantané source vérifiable. Les
fonctionnalités exactes de cette version restent donc non attribuées.

## 0.4.3 — 4 novembre 2026

- Persistance des propriétés d'héritage observées statiquement.
- Règles de gestion et ordonnancement de leurs fragments SQL.
- La couverture exacte de cette version est documentée dans les rapports
  d'audit ; aucun tag source historique n'est revendiqué.

## 0.4.0 à 0.4.2 — archives binaires conservées

Les paquets Debian ont survécu sans leurs arbres source correspondants. Ils
sont inventoriés dans `docs/RELEASE_ARCHIVE.md`.

## 0.1.0 à 0.3.0 — premiers prototypes binaires

Trois paquets Debian témoignent des premiers prototypes Qt. Les sources
exactes n'étant pas disponibles, OpenLooping ne publie pas de faux tags source
pour ces versions.
