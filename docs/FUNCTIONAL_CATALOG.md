# Catalogue fonctionnel et matrice de conformité

Statuts : `À AUDITER`, `SPÉCIFIÉ`, `EN COURS`, `IMPLÉMENTÉ`, `CONFORME`.
Priorités : `C` critique, `M` majeure, `N` normale. `À AUDITER/P1` indique un
premier scénario constaté, mais pas encore une spécification complète. Les
autres lignes restent `À AUDITER/P0`. Voir `docs/CLEAN_ROOM.md` pour les niveaux
de preuve.

## Application et documents

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| APP-001 | C | Nouveau document et valeurs par défaut | À AUDITER/P1 |
| APP-002 | C | Ouvrir, erreurs, versions et encodages | EN COURS |
| APP-003 | C | Enregistrer, enregistrer sous, écriture atomique | À AUDITER/P1 |
| APP-004 | M | Documents/fichiers récents | À AUDITER/P0 |
| APP-005 | C | Détection des modifications et confirmation de fermeture | À AUDITER/P0 |
| APP-006 | C | Annuler/rétablir et portée exacte des commandes | À AUDITER/P0 |
| APP-007 | M | Couper/copier/coller/dupliquer/supprimer | EN COURS |
| APP-008 | M | Sélectionner tout, sélection multiple et menus contextuels | EN COURS |
| APP-009 | M | Raccourcis clavier complets | À AUDITER/P0 |
| APP-010 | N | À propos, version, aide et liens | À AUDITER/P0 |
| APP-011 | C | Format de fichier `.loo` 4.1 et compatibilité historique | EN COURS |
| APP-012 | M | Préférences globales et propriétés par document | À AUDITER/P0 |
| APP-013 | M | Plusieurs fenêtres pour un document, cascade et mosaïque | EN COURS |
| APP-014 | M | Recherche de texte dans les vues et résultats | À AUDITER/P0 |

## Canevas et ergonomie graphique

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| CAN-001 | C | Création, déplacement, redimensionnement des objets | À AUDITER/P1 |
| CAN-002 | M | Zoom, niveaux, centrage et ajustement | EN COURS |
| CAN-003 | M | Panoramique et défilements vertical/horizontal | À AUDITER/P0 |
| CAN-004 | M | Roulette et Shift+roulette horizontal | EN COURS |
| CAN-005 | M | Grande zone graphique et limites configurables | À AUDITER/P0 |
| CAN-006 | M | Glisser-déposer et réordonnancement des rubriques | À AUDITER/P0 |
| CAN-007 | M | Routage, cassures et arrondi automatique des liens | À AUDITER/P0 |
| CAN-008 | M | Sélection mise en évidence et poignées | À AUDITER/P0 |
| CAN-009 | N | Alignement, distribution, superposition, verrouillage | À AUDITER/P0 |
| CAN-010 | M | Multi-sélection pour couleurs et propriétés communes | À AUDITER/P0 |
| CAN-011 | M | Couleurs globales, par type, par objet et par lien | À AUDITER/P0 |
| CAN-012 | M | Polices, tailles, fonds, traits et styles | À AUDITER/P0 |
| CAN-013 | M | Ascenseurs des grandes classes d'entités | À AUDITER/P0 |
| CAN-014 | N | Grille, magnétisme et repères, si présents | À AUDITER/P0 |
| CAN-015 | M | Multi-vue et multi-zoom | À AUDITER/P0 |
| CAN-016 | M | Grille : affichage, activation, grille fine, couleur et espacement | À AUDITER/P0 |
| CAN-017 | M | Ordre d'empilement et alignement des objets sélectionnés | À AUDITER/P0 |

## Objets conceptuels MCD

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| MCD-001 | C | Classe d'entités et toutes ses propriétés | À AUDITER/P1 |
| MCD-002 | C | Rubriques d'entités : ordre, type, taille, défaut | À AUDITER/P1 |
| MCD-003 | C | Identifiants simples et composés | À AUDITER/P1 |
| MCD-004 | C | Colonnes conceptuelles NULL/NOT NULL et UNIQUE | À AUDITER/P0 |
| MCD-005 | C | Associations binaires, n-aires et réflexives | À AUDITER/P1 |
| MCD-006 | C | Rubriques portées par une association | EN COURS/P1 |
| MCD-007 | C | Cardinalités minimales/maximales et toutes notations | À AUDITER/P1 |
| MCD-008 | C | Rôles, affichage et influence sur noms logiques | À AUDITER/P0 |
| MCD-009 | C | Identifiants relatifs et liens identifiants | À AUDITER/P0 |
| MCD-010 | M | Transformation association en entité | À AUDITER/P0 |
| MCD-011 | M | Entités fictives et condition d'activation | À AUDITER/P0 |
| MCD-012 | M | Entités externes et qualification base/schéma | À AUDITER/P0 |
| MCD-013 | C | Index et clés alternatives simples/composées ordonnées | À AUDITER/P0 |
| MCD-014 | C | Héritage, généralisation et spécialisation | EN COURS/P1 |
| MCD-015 | C | Contraintes d'héritage : totalité/exclusion/partition | EN COURS/P1 |
| MCD-016 | C | CIF complètes/incomplètes et représentation simplifiée | À AUDITER/P0 |
| MCD-017 | C | Multi-CIF, association pivot et unicité incomplète | À AUDITER/P0 |
| MCD-018 | C | CIF impliquant des associations réflexives | À AUDITER/P0 |
| MCD-019 | C | Contraintes inter-associations : tous types | À AUDITER/P0 |
| MCD-020 | M | SQL attaché aux contraintes et règles de gestion | EN COURS/P1 |
| MCD-021 | M | Textes, graphiques et images | À AUDITER/P0 |
| MCD-022 | M | Connexion des annotations par flux | À AUDITER/P0 |
| MCD-023 | M | Caractères exclusivement conceptuels et masquage | À AUDITER/P0 |
| MCD-024 | M | Validation, avertissements et erreurs de cohérence | À AUDITER/P0 |
| MCD-025 | M | Types de rubrique détaillés : texte, numérique, temporel, booléen, XML, géométrique/géographique et libre | EN COURS/P1 |
| MCD-026 | M | Taille, collation, précision, fuseau horaire, position dans les clés et compléments des rubriques | À AUDITER/P0 |
| MCD-027 | M | Propriétés graphiques et liens des illustrations : rectangle, ellipse, ligne, flèche, crayon, image et vignette | À AUDITER/P0 |

## Représentations, vues et urbanisation

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| VIEW-001 | C | Vue MCD Entité/Association | À AUDITER/P1 |
| VIEW-002 | M | Diagramme de classes UML complet | À AUDITER/P0 |
| VIEW-003 | C | MLD textuel temps réel | À AUDITER/P1 |
| VIEW-004 | C | MLD graphique et liens Crow's Foot | À AUDITER/P1 |
| VIEW-005 | M | Modèle organisationnel de flux | À AUDITER/P0 |
| VIEW-006 | M | Affichage types SGBD ou clés PK/AK/FK | À AUDITER/P0 |
| VIEW-007 | M | Affichage optionnel des noms d'index dans le MCD | À AUDITER/P0 |
| VIEW-008 | M | Associations/DF/CIF en représentation simplifiée | À AUDITER/P0 |
| VIEW-009 | C | Vues partielles d'un grand modèle | À AUDITER/P0 |
| VIEW-010 | C | Sous-modèles et liens modèle/sous-modèle | À AUDITER/P0 |
| VIEW-011 | M | Synchronisation instantanée de toutes les vues | À AUDITER/P0 |
| VIEW-012 | M | Positions et tailles partagées entre projections | À AUDITER/P0 |

## Transformation MCD vers MLD

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| MAP-001 | C | Entités, rubriques et identifiants vers relations | À AUDITER/P1 |
| MAP-002 | C | Associations 1:1 et choix du porteur de clé | À AUDITER/P0 |
| MAP-003 | C | Associations 1:N | À AUDITER/P1 |
| MAP-004 | C | Associations N:N et n-aires | À AUDITER/P1 |
| MAP-005 | C | Associations réflexives et rôles | À AUDITER/P0 |
| MAP-006 | C | Rubriques d'association | À AUDITER/P0 |
| MAP-007 | C | Identifiants relatifs | À AUDITER/P0 |
| MAP-008 | C | CIF complètes/incomplètes et clés produites | À AUDITER/P0 |
| MAP-009 | C | Multiples CIF sur pivot et théorème K(PK) | À AUDITER/P0 |
| MAP-010 | C | Stratégies de traduction des héritages | À AUDITER/P0 |
| MAP-011 | M | Migration des rubriques du surtype vers sous-types | À AUDITER/P0 |
| MAP-012 | M | Omission des entités fictives | À AUDITER/P0 |
| MAP-013 | M | Références aux entités externes | À AUDITER/P0 |
| MAP-014 | M | Libellés des identifiants automatiques | À AUDITER/P0 |
| MAP-015 | M | Noms personnalisés de clés étrangères | À AUDITER/P0 |
| MAP-016 | M | Noms logiques et conventions de casse | À AUDITER/P0 |
| MAP-017 | C | Détection des cycles et transformations impossibles | À AUDITER/P0 |

## SQL et SGBD

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| SQL-001 | C | Liste exacte des dialectes SGBD 4.1 | À AUDITER/P0 |
| SQL-002 | C | DDL mis à jour en temps réel | À AUDITER/P1 |
| SQL-003 | C | Export de script et encodage | À AUDITER/P0 |
| SQL-004 | C | Correspondances de types par dialecte | À AUDITER/P0 |
| SQL-005 | C | PK, FK, UNIQUE, index et noms de contraintes | À AUDITER/P1 |
| SQL-006 | C | Auto-incrément/séquences par dialecte | À AUDITER/P0 |
| SQL-007 | M | Valeurs par défaut et expressions | À AUDITER/P0 |
| SQL-008 | M | Quotage et caractères de protection | À AUDITER/P0 |
| SQL-009 | M | ON DELETE/UPDATE, cascades et clause libre | À AUDITER/P0 |
| SQL-010 | M | Contraintes déplacées dans ALTER TABLE finaux | À AUDITER/P0 |
| SQL-011 | M | Qualification base/schéma des objets externes | À AUDITER/P0 |
| SQL-012 | M | SQL personnalisé des règles/contraintes | À AUDITER/P0 |
| SQL-013 | C | Ordre de création compatible avec les dépendances | À AUDITER/P0 |
| SQL-014 | M | Configuration des SGBD personnalisés et types libres prédéfinis | À AUDITER/P0 |
| SQL-015 | M | Blocs SQL complémentaires : début/fin de script et de création de table | À AUDITER/P0 |

## Rétroconception

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| REV-001 | C | Saisie/collage/chargement d'instructions DDL | À AUDITER/P0 |
| REV-002 | C | Dialectes reconnus et sélection | À AUDITER/P0 |
| REV-003 | C | Tables, colonnes, types et nullabilité | À AUDITER/P0 |
| REV-004 | C | PK simples/composées | À AUDITER/P0 |
| REV-005 | C | FK simples/composées et réflexives | À AUDITER/P0 |
| REV-006 | C | UNIQUE, index et contraintes nommées | À AUDITER/P0 |
| REV-007 | M | CREATE/ALTER TABLE et ordre indépendant | À AUDITER/P0 |
| REV-008 | M | Tolérance espaces, lignes, commentaires, `;` | À AUDITER/P0 |
| REV-009 | M | Reconstruction conceptuelle des associations | À AUDITER/P0 |
| REV-010 | M | Placement initial et gestion des superpositions | À AUDITER/P0 |
| REV-011 | C | Diagnostics et conservation des éléments inconnus | À AUDITER/P0 |
| REV-012 | M | Remplacement configuré des caractères dans les noms de rubriques | À AUDITER/P0 |

## Échanges, sortie et impression

| ID | P | Capacité à caractériser intégralement | État |
|---|---:|---|---|
| IO-001 | C | Export image : BMP, PNG, GIF et autres formats éventuels, dimensions, fond et qualité | À AUDITER/P0 |
| IO-002 | M | Copie graphique vers presse-papiers | À AUDITER/P0 |
| IO-003 | M | Impression, pagination, orientation et échelle | À AUDITER/P0 |
| IO-004 | M | Export PDF via capacités observées/système | À AUDITER/P0 |
| IO-005 | M | Export XML du MLD textuel | À AUDITER/P0 |
| IO-006 | M | Import/export ou copie de sous-modèles | À AUDITER/P0 |
| IO-007 | M | Conservation des couleurs, positions et cassures | À AUDITER/P0 |

## Linux, accessibilité et distribution

| ID | P | Capacité attendue | État |
|---|---:|---|---|
| LNX-001 | C | Paquet `.deb` installable et désinstallable proprement | À AUDITER/P0 |
| LNX-002 | C | Ubuntu LTS et Debian stable | À AUDITER/P0 |
| LNX-003 | M | Sessions Wayland et X11 | À AUDITER/P0 |
| LNX-004 | M | HiDPI et plusieurs écrans | À AUDITER/P0 |
| LNX-005 | M | AZERTY/QWERTY et saisie Unicode | À AUDITER/P0 |
| LNX-006 | M | Parcours complet au clavier | À AUDITER/P0 |
| LNX-007 | M | Lecteurs d'écran et noms accessibles Qt | À AUDITER/P0 |
| LNX-008 | M | Type MIME et ouverture par double-clic | À AUDITER/P0 |
| LNX-009 | M | Données utilisateur selon XDG | À AUDITER/P0 |
| LNX-010 | C | Sauvegarde automatique et récupération après incident | À AUDITER/P0 |

## Registre des éléments encore inconnus

Cette section est volontairement bloquante pour la déclaration de complétude :

- liste exacte des menus, actions, raccourcis et menus contextuels ;
- liste exacte des dialectes et options de génération SQL ;
- format, extension et variantes historiques des fichiers ;
- liste complète des types de contraintes inter-associations ;
- propriétés exactes du MOF, des flux, règles, images et textes ;
- formats et options exactes d'export/ impression ;
- préférences générales, valeurs par défaut et limites numériques ;
- comportements de chaque fenêtre de propriétés et messages d'erreur.

Ils seront levés par l'audit systématique de l'exécutable 4.1, pas par intuition.

