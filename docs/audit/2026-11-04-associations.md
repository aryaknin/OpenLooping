# Audit 04 — association binaire et cardinalités de base

Date : 4 novembre 2026. Référence : `Looping_32bits.exe` 4.1 du manifeste.
Observation de l'interface sous Wine 10.0 et Xvfb, sans reprise de code ni de
ressource du logiciel. Les captures et modèles cités résident dans
`work/analysis-raw/`, exclu de la distribution.

## Cas témoin

Le modèle `sample-client-commande-unlinked.loo` contient deux entités non
reliées : `Client` avec `nom` identifiant, et `Commande` avec `idCommande`
identifiant. Pour cette seconde rubrique, le dialogue proposait `Entier` sur
32 bits (`audit04-idcommande-fields.png`). Ce fichier mesure 7 309 octets,
SHA-256 `60a3eefefad4b7e6b40057e7a119c67038205fc0703763233a0fd375c390d9e1`.

La palette MCD permet de poser une association indépendante sur le canevas.
Son nom initial est `Asso 1` et elle est représentée par une ellipse cyan
(`audit04-association-unlinked.png`). Un double-clic ouvre sa fiche
(`audit04-association-props.png`) : `Nom`, `Nom logique`, liste de rubriques,
`Commentaire` et réglages graphiques. La liste possède les colonnes `Nom
conceptuel`, `Nom logique`, `Type`, `NOT NULL`, `UNIQUE`, ainsi que les actions
Ajouter, Dupliquer, Modifier, Renommer, Supprimer et réordonner. Ces commandes
ont été *visibles*, mais pas encore testées sur une rubrique d'association.

Après renommage en `passe`, l'association et les deux entités ont été
enregistrées dans `sample-client-commande-asso-unlinked.loo` (9 438 octets,
SHA-256 `0e10acfee07a05d0af37f25b2e6bf08ad5f0b2cefb00b5184735996a4d153ebe`).
La réouverture restitue l'association indépendante
(`audit04-resume-canvas.png`). Ce test confirme la création, le renommage et
la persistance d'une association sans lien ; il ne prouve pas encore les
associations binaires, n-aires ou réflexives.

## Création et fiche d'un lien

La palette expose une commande `Lien`. La séquence reproductible est : activer
l'outil, cliquer l'entité, puis cliquer l'association. Un glisser-déposer ne
constitue donc pas le protocole témoin. Le premier clic laisse l'outil en attente
de sa cible ; le second crée le lien avec la cardinalité `0,n` par défaut
(`audit04-link-after-source.png`, `audit04-link-two-clicks.png`).

Après retour à l'outil de sélection, un double-clic sur le lien ouvre une fiche
(`audit04-link-properties.png`). Les cardinalités prédéfinies visibles sont
`0,1`, `1,1`, `0,n` et `1,n`. Une cinquième option expose des champs `Min` et
`Max` libres. La fiche contient également « Place clé étrangère dans clé
primaire associée » avec une valeur numérique, ainsi qu'un rôle doté d'un
`Libellé`. Seules les cardinalités décrites ci-dessous ont été modifiées ; les
champs de clé et de rôle restent à caractériser.

## Cas `0,n — 0,n`

Les deux liens nouvellement créés prennent indépendamment la valeur `0,n`
(`audit04-binary-default.png`). Le fichier
`sample-client-commande-asso-default-links.loo` mesure 14 598 octets, SHA-256
`3415921472bb464726b0084ff3b79582ae8e5e9a9691c10b182bee88e3318587`.

Le MLD graphique transforme l'association `passe` en relation contenant `nom`
et `idCommande`, tous deux clés étrangères et membres de sa clé primaire
(`audit04-mld-default-links.png`). Le MLD textuel affiche :

```text
Client = (nom VARCHAR(50));
Commande = (idCommande INT);
passe = (#nom, #idCommande);
```

Les soulignements graphiques désignent les clés primaires ; le caractère `#`
désigne ici une clé étrangère. Le LDD observé crée les deux tables d'entités,
puis :

```sql
CREATE TABLE passe(
    nom VARCHAR(50),
    idCommande INT,
    PRIMARY KEY(nom, idCommande),
    FOREIGN KEY(nom) REFERENCES Client(nom),
    FOREIGN KEY(idCommande) REFERENCES Commande(idCommande)
);
```

Captures : `audit04-default-mld-sql-panels.png` et
`audit04-default-sql-right.png`.

## Cas `0,n — 1,1` et `0,n — 0,1`

Avec `Commande` du côté `1,1`, la relation `passe` disparaît du MLD et la clé
`nom` migre dans `Commande`. Le MLD textuel devient
`Commande = (idCommande INT, #nom);`. Le LDD ajoute `nom VARCHAR(50) NOT NULL`
et `FOREIGN KEY(nom) REFERENCES Client(nom)` à la table `Commande`
(`audit04-cardinality-0n-11.png`). Le fichier correspondant mesure 14 598
octets, SHA-256
`7ec24bbe90ca1540e03e610b48aa00d131df19f54e1c71ea833ae6888512378c`.

En remplaçant uniquement `1,1` par `0,1`, le même attribut et la même clé
étrangère restent dans `Commande`, mais `NOT NULL` disparaît du LDD
(`audit04-cardinality-0n-01.png`). Le fichier correspondant mesure 14 598
octets, SHA-256
`b949a6a8b40d0ba5ae52385d01d07697ccb4c53232090b36756b12694c4f8631`.

Ces deux tests établissent, pour ce micro-modèle, la traduction du minimum 1 en
`NOT NULL` et du minimum 0 en colonne nullable. Ils ne suffisent pas à établir
toutes les règles des associations 1:1, des clés composites ou des dialectes
SQL.

## Incident d'environnement résolu

Les premières sessions Xvfb séparées étaient nettoyées lorsque leur commande
rendait la main, ce qui interrompait l'observation. Xvfb et Wine ont ensuite été
placés dans une même session durable. Le modèle sauvegardé a pu être rouvert et
les expériences ci-dessus ont été reproduites. Cet incident concernait le
dispositif d'audit, pas un comportement attribuable à Looping.

## Prochains tests contrôlés

1. Tester `1,n`, les valeurs Min/Max libres et leur validation.
2. Caractériser les deux côtés `0,1`/`1,1`, le choix du porteur de clé et les
   avertissements des configurations ambiguës.
3. Tester le rôle, l'inclusion de la clé étrangère dans la clé primaire et les
   clés composées.
4. Étendre aux associations réflexives, n-aires et avec rubriques.
5. Répéter les sorties avec chaque dialecte SQL pris en charge.
