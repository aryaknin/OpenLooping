# Roadmap de parité Looping 4.1

Référence cible : **Looping 4.1 du 7 mars 2025, éditions Windows 32 et 64 bits**.

Cette roadmap distingue la découverte de la livraison. Une fonction trouvée tard
pendant l'audit devient une ligne du catalogue avant d'être développée : ainsi,
elle ne peut pas disparaître entre l'analyse et le code.

## Critère de parité

La version 1.0 ne sera déclarée complète que si :

1. tous les menus, fenêtres, outils, raccourcis et options de Looping 4.1 ont été
   inventoriés sur les deux exécutables ;
2. chaque élément possède un identifiant dans le catalogue fonctionnel ;
3. tous les scénarios ont un niveau de preuve P2 minimum ;
4. toutes les fonctions applicables ont un test différentiel P3 ;
5. les fichiers de référence sélectionnés s'ouvrent, se réenregistrent et
   produisent des MLD, UML et DDL sémantiquement équivalents ;
6. aucun élément critique ou majeur n'est ouvert ;
7. le paquet `.deb` passe les contrôles Debian et fonctionne sur les versions
   Ubuntu/Debian retenues dans la matrice de support.

## Phase 0 — Gouvernance et laboratoire clean room

- Fixer l'empreinte SHA-256 des exécutables 4.1 et archiver leur provenance hors
  du dépôt.
- Capturer l'arborescence complète des menus, fenêtres, onglets, menus contextuels,
  raccourcis, messages d'erreur et valeurs par défaut.
- Créer un corpus de micro-modèles : un fichier par notion et cas limite.
- Installer REA et Ghidra dans un environnement isolé.
- Créer les modèles de fiches d'observation et la matrice exigences/tests.
- Documenter les limites juridiques et l'origine de chaque ressource visuelle.

**Sortie :** catalogue fermé provisoire, captures d'état et corpus v1.

## Phase 1 — Socle natif Qt 6

- Application Qt Widgets, C++20, CMake et tests Qt.
- Modèle de document indépendant de l'interface et sérialisation versionnée.
- Commandes transactionnelles avec annuler/rétablir illimités et état modifié.
- Sélection simple/multiple, presse-papiers, glisser-déposer et raccourcis.
- Canevas performant : zoom, panoramique, défilement, grandes surfaces, impression.
- Préférences, thèmes, couleurs, unités et persistance des réglages.
- Journal d'erreurs, récupération après incident et fichiers atomiques.
- Intégration continue et analyse mémoire/sanitizers.

**Sortie :** éditeur vide robuste, installable et testé.

## Phase 2 — Noyau MCD Entité/Association

- Classes d'entités, associations binaires/n-aires/réflexives et rubriques.
- Identifiants simples/composés, types, valeurs par défaut, NULL/NOT NULL, UNIQUE.
- Cardinalités, rôles, identifiants relatifs et transformations association-entité.
- Clés alternatives et index simples/composés ordonnés.
- Entités fictives et entités appartenant à un modèle externe.
- Héritage, généralisation/spécialisation et contraintes associées.
- CIF complètes/incomplètes, multiples, pivots et réflexives.
- Contraintes inter-associations et code SQL complémentaire.
- Règles de gestion, textes, images/graphiques et flux connectables.
- Validation continue et diagnostics pédagogiques.

**Sortie :** tous les objets conceptuels éditables et sauvegardés.

## Phase 3 — Moteur graphique et présentation

- Routage et cassures de liens, arrondis, styles et couleurs individualisées.
- Libellés de rôles/cardinalités, points d'ancrage et liens identifiants.
- Ordre et déplacement des rubriques sur le modèle.
- Ascenseurs internes des grandes entités et zones graphiques étendues.
- Alignement, distribution, premier/arrière-plan et mise en page.
- Multi-vues, multi-zoom, couleurs par catégorie et sélection mise en évidence.
- Vues partielles pour urbanisation, modèles et sous-modèles liés.
- Modes simplifiés des associations avec DF/CIF.

**Sortie :** rendu MCD comparable et utilisable sur de grands sujets de classe.

## Phase 4 — Projections synchronisées

- Diagramme de classes UML synchronisé.
- MLD textuel mis à jour en temps réel.
- MLD graphique avec notation Crow's Foot.
- Modèle organisationnel de flux (MOF).
- Affichage configurable des types, clés PK/AK/FK et noms d'index.
- Navigation instantanée entre représentations avec une seule source métier.
- Export XML du MLD textuel.

**Sortie :** toutes les représentations officielles couvertes.

## Phase 5 — Transformation conceptuel → logique

- Entités et identifiants vers tables et clés primaires.
- Associations 1:1, 1:N, N:N, n-aires et réflexives.
- Associations porteuses de rubriques.
- Identifiants relatifs et entités fictives.
- Clés alternatives issues des CIF, y compris multi-CIF incomplètes.
- Stratégies d'héritage et migration optionnelle des rubriques du surtype.
- Entités externes, préfixes de schéma/base et exclusions de génération.
- Personnalisation des identifiants automatiques et clés étrangères.
- Rôles utilisés comme suffixes ou noms de clés étrangères.
- Noms logiques et conventions Snake/camel/Pascal à caractériser précisément.

**Sortie :** MLD sémantiquement équivalent sur le corpus complet.

## Phase 6 — Génération SQL/DDL

- DDL temps réel et export de scripts.
- Dialectes SGBD recensés depuis l'interface 4.1, sans en supposer la liste.
- Types, auto-incrément, valeurs par défaut, NULL, UNIQUE, PK, FK et index.
- Protection/quotage des identifiants et caractères conceptuels masqués.
- Cascades référentielles prédéfinies et clauses personnalisées.
- Contraintes en ligne ou déplacées dans des `ALTER TABLE` finaux.
- Code SQL des règles et contraintes inter-associations.
- Ordonnancement déterministe, encodage et fins de ligne.
- Suite d'exécution des scripts sur chaque SGBD libre supporté.

**Sortie :** DDL équivalent et exécutable pour chaque dialecte annoncé.

## Phase 7 — Rétroconception DDL

- Éditeur/import de DDL et choix de police/présentation observé.
- Analyse lexicale tolérante aux espaces, retours ligne et points-virgules.
- Reconstruction des tables, colonnes, PK, FK, UNIQUE, index et contraintes.
- Reconstruction des associations, réflexives, cardinalités et identifiants.
- Placement initial sans superposition, puis réorganisation manuelle.
- Diagnostics précis pour les constructions non reconnues.
- Aller-retour DDL → MCD → DDL testé par dialecte.

**Sortie :** rétroconception conforme aux capacités et limites de la 4.1.

## Phase 8 — Fichiers, échanges et compatibilité

- Nouveau, ouvrir, enregistrer, enregistrer sous et fichiers récents.
- Lecture du format Looping 4.1 et des versions historiques présentes dans le
  corpus ; écriture compatible uniquement si l'audit confirme que c'est sûr.
- Import/export des sous-modèles et références externes.
- Export image dans tous les formats/réglages observés.
- Export presse-papiers, impression et PDF via le système Linux.
- Préservation des positions, couleurs, cassures, vues et préférences de document.
- Messages et stratégie de migration pour les versions inconnues.

**Sortie :** échange fiable avec les postes Windows de la classe.

## Phase 9 — Qualité Linux et paquet Debian

- Paquets `.deb` amd64 et arm64 si toutes les dépendances le permettent.
- Lanceur, icône originale, type MIME, association de fichiers et désinstallation.
- Mode Wayland et X11, écrans HiDPI, claviers AZERTY/QWERTY et thèmes clair/sombre.
- Accessibilité clavier, focus visible et technologies d'assistance Qt.
- Tests Ubuntu LTS et Debian stable dans machines propres.
- Documentation utilisateur française et tutoriels de migration.
- AppImage/Flatpak envisagés après stabilisation du `.deb`.

**Sortie :** version candidate utilisable en cours sans privilèges administrateur
après installation.

## Phase 10 — Validation de parité et version 1.0

- Rejouer l'intégralité du catalogue sur Looping 4.1 et la réimplémentation.
- Comparer visuellement les rendus avec tolérances explicites.
- Comparer structurellement MLD, UML, XML et DDL plutôt que seulement le texte.
- Faire tester par enseignants et étudiants sur de vrais sujets anonymisés.
- Corriger tous les écarts critiques/majeurs et publier le rapport de conformité.
- Geler le format, signer le paquet et publier les sources reproductibles.

## Ordre de livraison pédagogique

Les versions intermédiaires restent honnêtes sur leur périmètre :

- `0.1` : socle, canevas et document ;
- `0.2` : MCD de base utilisable ;
- `0.3` : notions Merise avancées ;
- `0.4` : MLD/UML/MOF ;
- `0.5` : génération SQL ;
- `0.6` : compatibilité de fichiers et exports ;
- `0.7` : rétroconception ;
- `0.8` : grands modèles, vues et finition ;
- `0.9` : audit de parité complet ;
- `1.0` : aucune exigence critique ou majeure manquante.

