# OpenLooping

[![Linux CI](https://github.com/aryaknin/OpenLooping/actions/workflows/ci.yml/badge.svg)](https://github.com/aryaknin/OpenLooping/actions/workflows/ci.yml)

OpenLooping est une réimplémentation indépendante **clean room** d'un éditeur
de modèles conceptuels de données pour GNU/Linux. Elle vise en priorité les
classes utilisant la méthode Merise sur Debian et Ubuntu, lorsque certains
étudiants ne peuvent pas exécuter le logiciel Windows employé en cours.

Le projet est écrit en **C++20 avec Qt 6**, produit un paquet `.deb` et conserve
un format ouvert `.lrea`. Son objectif est une compatibilité fonctionnelle et
pédagogique vérifiable, sans réutiliser de code source, ressource graphique ou
document propriétaire.

> OpenLooping est un projet indépendant, non affilié et non approuvé par les
> auteurs ou éditeurs du logiciel de référence. Le nom du logiciel de référence
> n'est utilisé dans la documentation que pour décrire la compatibilité.

## État actuel — 0.6.1

Le prototype fournit notamment :

- un canevas MCD Qt avec entités, rubriques, associations et liens ;
- les cardinalités `0,1`, `1,1`, `0,n` et `1,n` ;
- des identifiants simples/composés et les propriétés `NOT NULL` et `UNIQUE` ;
- la règle cohérente « identifiant implique NOT NULL » dans l'interface, le
  modèle et la persistance ;
- une palette complète de types texte, numériques, temporels, booléens, XML,
  géométriques, géographiques et personnalisés ;
- un MLD textuel et une génération SQL réactualisés après modification ;
- les transformations observées N:N, 1:N et 0,1 ;
- les héritages et règles de gestion déjà caractérisés ;
- annuler/rétablir, presse-papiers, sélection multiple, menus contextuels,
  zoom, ajustement et fenêtres internes ;
- un rendu adaptatif avec grille, boîtes dimensionnées au contenu, liens ancrés
  aux contours et cardinalités lisibles ;
- la sauvegarde au format JSON ouvert `.lrea` ;
- l'import protégé des archives natives `.loo` et des sauvegardes numérotées
  `.lo1` à `.lo9` proposées par le sélecteur de fichiers ;
- un paquet Debian amd64 construit avec CPack.

Cette version n'est **pas** présentée comme une parité complète. Les références
directes des arcs natifs, certains héritages importés, CIF, UML, MOF,
rétroconception SQL, export graphique et plusieurs préférences restent à
implémenter ou valider. Le détail se trouve dans le
[catalogue fonctionnel](docs/FUNCTIONAL_CATALOG.md).

## Import des fichiers natifs

L'import reconnaît les archives par leur contenu et ouvre toujours le fichier
Windows en lecture seule. Pour les modèles riches, OpenLooping récupère les
entités, rubriques, identifiants, positions, associations et cardinalités déjà
décodées. Lorsque l'extrémité d'une branche est déterminée spatialement plutôt
que par une référence d'objet complètement comprise, l'interface l'annonce au
lieu de masquer cette limite.

Le fichier source n'est jamais écrasé : l'enregistrement crée un document
`.lrea` distinct.

## Compiler sur Debian ou Ubuntu

Prérequis : CMake 3.24+, Ninja, un compilateur C++20 et Qt 6.4+ avec les modules
Core, Widgets et Test.

```bash
sudo apt install cmake ninja-build g++ qt6-base-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/openlooping
```

Ouvrir directement un modèle sans installer le paquet :

```bash
./build/openlooping "/chemin/vers/modele.lo1"
```

Construire le paquet Debian :

```bash
cpack --config build/CPackConfig.cmake -G DEB
```

## Organisation du dépôt

```text
src/core/       modèle, persistance, import natif et génération relationnelle
src/ui/         fenêtre, canevas et dialogues Qt
tests/          tests automatisés du noyau et de l'interface
docs/audit/     observations reproductibles et limites connues
packaging/      intégration au bureau Linux
```

Les binaires de référence, captures privées, fichiers de cours et sorties
brutes d'analyse sont volontairement exclus du dépôt.

## Méthode clean room

Le développement suit quatre règles :

1. observer uniquement des artefacts légalement obtenus et des scénarios
   reproductibles ;
2. consigner les faits, leurs empreintes et leurs limites ;
3. écrire une implémentation originale en C++/Qt ;
4. distinguer dans le code et la documentation les faits observés, les
   inférences et les inconnues.

Voir le [protocole clean room](docs/CLEAN_ROOM.md), le
[manifeste des références](docs/REFERENCE_MANIFEST.md) et les
[outils d'analyse](docs/TOOLING.md).

## Documentation

- [Roadmap vers la parité](ROADMAP.md)
- [Catalogue fonctionnel](docs/FUNCTIONAL_CATALOG.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Historique des versions](CHANGELOG.md)
- [Registre des paquets historiques](docs/RELEASE_ARCHIVE.md)
- [Contribuer](CONTRIBUTING.md)
- [Sécurité](SECURITY.md)

## Historique Git reconstitué

Le dossier de travail initial ne contenait aucun dépôt Git exploitable. Les
paquets Debian `0.1.0` à `0.6.1` ont survécu, mais pas les arbres source de
toutes ces versions. L'historique publié regroupe donc le code authentique
disponible en étapes techniques cohérentes et marque ses commits de
reconstruction. Aucun faux instantané source ancien n'est présenté comme
authentique. Les empreintes des paquets conservés sont publiées séparément.

## Contribuer

Les contributions sont bienvenues, en particulier pour les tests Debian/Ubuntu,
l'accessibilité, le routage graphique, les transformations Merise et les
formats natifs. Consultez [CONTRIBUTING.md](CONTRIBUTING.md) avant toute PR.

La version `1.0` ne sera annoncée que lorsque les exigences critiques et
majeures du catalogue auront une preuve reproductible et un test de conformité.
