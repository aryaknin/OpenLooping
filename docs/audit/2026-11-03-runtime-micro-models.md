# Audit 03 — micro-modèles Entité, rubrique et identifiant

Date : 3 novembre 2026. Référence : `Looping_32bits.exe` 4.1, empreinte du
manifeste. Exécution sous Wine 10.0, écran virtuel Xvfb `:97`, préfixe isolé du
projet. Fichiers `.loo` et captures cités ci-dessous dans
`work/analysis-raw/`, hors du dépôt distribué.

## Protocole différentiel

Chaque nouveau cas part d'un fichier sauvegardé et est écrit sous un autre nom.
La copie inchangée de `sample-one-entity.loo`, ouverte puis enregistrée sous
`sample-one-entity-roundtrip.loo`, est identique octet pour octet (SHA-256
`370a8d6855b6715566b4c9244f0d37628719ace0b65c6664ca838f2ecb896f22`).
Cela vérifie la stabilité de ce cas simple, pas celle de tous les documents.

| Cas | Modification par rapport au parent | Taille | SHA-256 |
|---|---|---:|---|
| `sample-one-entity.loo` | Entité par défaut « Entité 1 », sans rubrique | 4 719 | `370a8d6855b6715566b4c9244f0d37628719ace0b65c6664ca838f2ecb896f22` |
| `sample-entity-client.loo` | Nom conceptuel remplacé par « Client » | 4 711 | `1a3b6043c2d7c1fbe5c4f21acd7b6a333f1da7062b95656d09eb951008dfa6ec` |
| `sample-client-logical-custom.loo` | Nom logique de « Client » remplacé par `clients` | 4 725 | `1e7f4cc05e793f7ca3e26dc5129f28dcb2adcd25a7c2bb273ac6eacbd5b54b45` |
| `sample-client-one-field.loo` | Une rubrique `nom`, réglages par défaut | 4 957 | `246b04a2cd59a5095a6c8317613fb4d46629cb7d6385b21a2f3a1484fff06035` |
| `sample-client-nom-identifier.loo` | La rubrique `nom` devient identifiante | 4 957 | `bad0d6829e072ff4c91c5aca6f2e8ac6379e05940510c3be23134dea81b6bcae` |

L'enregistrement d'un cas modifié peut changer plusieurs zones du flux binaire,
même à taille égale. Les différences brutes ne permettent donc pas encore de
localiser sûrement chaque propriété : une analyse du graphe de sérialisation
sera nécessaire avant d'écrire un lecteur `.loo`.

## Observations de l'interface

1. Le modèle à une entité se rouvre et restitue « Entité 1 » au même endroit
   (`audit03-opened-entity.png`). La commande « Enregistrer sous » permet une
   copie inchangée et stable.
2. Lorsqu'on saisit « Client » dans « Nom », « Nom logique » affiche encore
   `Entité_1` avant validation (`audit03-rename-typed.png`). Après validation et
   réouverture de la fiche, les deux noms affichent « Client »
   (`audit03-rename-after.png`). Le nom logique a donc été recalculé au plus
   tard à la validation dans ce scénario.
3. Le dialogue « Ajouter » une rubrique propose par défaut : caractères ASCII,
   texte variable, longueur 50, sans Identifiant, NOT NULL ni UNIQUE. Il expose
   aussi les familles Unicode, Binaire, Numérique, Date/Heure et Autre ; leur
   comportement n'est pas encore caractérisé (`audit03-rubrique-dialog.png`).
4. Une rubrique nommée `nom` apparaît avec nom logique `nom` et type
   `VARCHAR(50)` dans la fiche d'entité (`audit03-rubrique-line.png`) et dans
   le canevas MCD (`audit03-onefield-canvas.png`).
5. Cocher « Identifiant » fait apparaître « Place dans clé : 1 » et masque les
   cases NOT NULL et UNIQUE dans le dialogue (`audit03-identifier-checked.png`).
   Après validation, `nom` est souligné dans les vues MCD et MLD. Cela ne
   démontre pas encore les règles des clés composites.
6. Le ruban MLD graphique possède les modes « Standard » et « Crow's foot ».
   La palette « Fenêtres » propose les panneaux Propriétés, MLD textuel et
   Script LDD (`audit03-mld-menu.png`, `audit03-windows-menu.png`).

## Sorties constatées pour le même modèle

Sans identifiant (`sample-client-one-field.loo`), le panneau MLD affiche
`Client = (***CLE ABSENTE***, nom VARCHAR(50));` en marquant l'absence de clé
en rouge. Le panneau LDD affiche : « Impossible de générer le script SQL :
voir les erreurs dans le MLD textuel » (`audit03-noid-panels.png`).

Avec `nom` identifiant (`sample-client-nom-identifier.loo`), le panneau MLD
affiche `Client = (nom VARCHAR(50));`, avec `nom` souligné. Le LDD affiché est :

```sql
CREATE TABLE Client(
    nom VARCHAR(50),
    PRIMARY KEY(nom)
);
```

L'interface ne précise pas encore, dans ce scénario, le dialecte SQL actif.
L'absence de `NOT NULL` explicite est une observation du script affiché, pas
une recommandation de génération pour notre application. Captures :
`audit03-mld-text.png`, `audit03-sql-panel.png`.

## Incident d'exécution et inconnues

Dans une session où deux documents et les panneaux MLD/LDD étaient ouverts,
`Ctrl+O` a précédé la fin du processus Wine (code de sortie 143), laissant
l'écran virtuel noir. Une session neuve a pu ouvrir le même fichier sans
incident. Ce test ne permet pas d'attribuer la fermeture à Looping, Wine ou
l'orchestration de l'écran virtuel ; il faudra le reproduire sous contrôle.

À caractériser ensuite : types et tailles limites, clés composites, règles de
nom logique, valeur par défaut du dialecte SQL, export de texte exact,
associations et cardinalités, validation des fichiers malformés.
