# Audit 02 — premiers parcours réels sous Wine

Date : 3 novembre 2026. Cible : `Looping_32bits.exe` 4.1 du manifeste.
Méthode : exécution sur Wine 10.0 dans un écran Xvfb `:97` et un préfixe
Wine isolé sous `work/analysis-raw/wineprefix`. Les captures et fichiers `.loo`
bruts restent sous `work/analysis-raw/`, exclus du dépôt distribué.

## Conditions et limites

- L'édition 64 bits a quitté immédiatement lors de ce premier essai avec
  `X Error ... BadWindow ... X_UnmapWindow`. L'édition 32 bits a démarré dans
  le même écran virtuel. Cela ne démontre pas un défaut du binaire 64 bits :
  l'origine exacte (Wine, configuration graphique, binaire) reste à isoler.
- L'écran d'accueil montre une annonce de livre et se ferme avec `Échap`.
- Aucun test de compatibilité du `.loo` avec la version 64 bits ou une version
  historique n'a encore été effectué.

## Scénarios observés

| ID | Entrée et actions | Résultat observé | Preuve |
|---|---|---|---|
| R01 | Démarrer l'édition 32 bits, fermer l'accueil | Fenêtre principale avec ruban « Looping », canevas gris sans document | `looping-32-start.png`, `looping-main.png` |
| R02 | Ouvrir le menu applicatif | Nouveau, Ouvrir, Enregistrer, Enregistrer sous, Fermer, Imprimer, Exporter, Paramètres LDD, Urbanisation, Rétroconception, À propos ; zone des documents récents | `looping-menu.png` |
| R03 | Nouveau, puis choisir « No » à la reprise des propriétés | Document vierge nommé `Looping1`. La question précise qu'un `Looping.loo` dans le même répertoire que l'exécutable fournirait automatiquement les propriétés | `looping-new.png`, `looping-blank.png` |
| R04 | `Ctrl+S`, chemin Windows absolu dans le dialogue Wine | Fichier `sample-empty.loo` de 2 594 octets. Le type proposé est `Looping Files (*.loo)` | `looping-save-dialog.png`, `sample-empty.loo` |
| R05 | Ouvrir les palettes du ruban | Formalisme E/A : Standard, Flèche pour DF, Asso simplifiée. Palette MCD : Entité, Association, Héritage, Contrainte, CIF, Règle, Lien, Cassure, Sous-modèle | `looping-ea-tools.png`, `looping-mcd-palette.png` |
| R06 | MCD → Entité, puis clic sur le canevas | Entité par défaut « Entité 1 », rectangle jaune, sélection par poignées | `looping-entity-created.png` |
| R07 | Double-clic sur cette entité | Fiche avec Nom, Nom logique, tableau Rubriques (`Id`, `Nom conceptuel`, `Nom logique`, `Type`, `NOT NULL`, `UNIQUE`), boutons Ajouter/Dupliquer/Modifier/Renommer/Supprimer, ordre, clés, Commentaire, option « Classe d'entités fictive » | `looping-entity-properties.png` |
| R08 | Nouveau document indépendant, « No », une seule Entité, `Ctrl+S` | `sample-one-entity.loo` de 4 719 octets, sans autre objet ; second `Ctrl+S` sans changement : même SHA-256 | `looping-one-entity.png`, `sample-one-entity.loo` |

Le test R04 a d'abord utilisé un chemin avec `/` dans le champ du nom de
fichier. Wine a affiché une erreur de caractère interdit. La forme
`Z:\home\...\sample-empty.loo` a fonctionné. Ce point relève du dialogue Wine,
pas nécessairement de Looping sous Windows.

## Premières contraintes du format `.loo`

Les deux fichiers sont binaires, non XML/texte et sans signature ZIP. Les
2 592 premiers octets sont identiques entre le modèle vide et celui avec une
entité. L'octet d'offset `0xA20` passe de `00` à `04`, puis le fichier avec
entité contient notamment les identifiants de classe `CDrawEntite` et
`CDrawTexte`. Ces observations ne suffisent pas à assigner une sémantique
fiable à tous les champs ; il faut créer davantage de paires contrôlées.

Empreintes SHA-256 :

| Fichier | Taille | SHA-256 |
|---|---:|---|
| `sample-empty.loo` | 2 594 | `addcfc8bd3430e60659f5ea9267135105a2755050c2518abd737c57a5e0dab94` |
| `sample-one-entity.loo` | 4 719 | `370a8d6855b6715566b4c9244f0d37628719ace0b65c6664ca838f2ecb896f22` |

## Étapes de vérification suivantes

1. Produire des micro-modèles avec une seule propriété changée à la fois : nom,
   nom logique, rubrique, identifiant, type, couleur, position.
2. Relever les dialogues Association et Lien, puis comparer MCD, MLD et SQL.
3. Tester ouverture/fermeture, annulation/rétablissement et gestion des
   modifications avant de spécifier ces comportements.
4. Réessayer la version 64 bits avec un environnement Wine propre, avant toute
   conclusion sur son comportement.
