# Audit 01 — ressources statiques de Looping 4.1

Date : 3 novembre 2026. Cible : les deux exécutables du manifeste
`docs/REFERENCE_MANIFEST.md`. Méthode : inventaire PE en lecture seule avec REA,
puis décodage reproductible des ressources Windows par
`work/analysis-raw/pe_resource_inventory.py`. Aucun exécutable n'a été lancé.

## Preuves et périmètre

- REA `inspect_pe_resources`, exécutable 64 bits :
  `ev_f890e370c9bb16c00442aef4df8cffa7308c2cc1f85575c491e0ff39446a10e7`.
  Inventaire complet de 1 048 ressources sur 2 113 entrées de répertoires.
- REA `inspect_pe_imports`, exécutable 64 bits :
  `ev_b3e03f4a700c10a7e9b703cfd47ce969df9e1422fcc1cc298c7630891dd5d1f7`.
  19 DLL et 719 fonctions importées, sans erreur de lecture.
- SHA-256 des deux binaires : voir le manifeste. Les ressources `MENU`,
  `DIALOG`, `STRING` sont identiques après décodage dans les éditions 32 et 64 bits.
  Cela ne prouve pas une équivalence du code ou du comportement.

Les ressources Windows décrivent des éléments embarqués. Certaines proviennent
du framework MFC et peuvent ne jamais apparaître dans l'interface de Looping.
Un libellé seul ne démontre ni l'accessibilité ni l'effet d'une commande.

## Inventaire d'interface établi

| Famille | Fait statique constaté | Vérification suivante |
|---|---|---|
| Fichier | Association déclarée aux documents `.loo` ; commandes nouveau, ouvrir, fermer, enregistrer, enregistrer sous, impression, aperçu et documents récents | Créer un document et caractériser les octets `.loo`, les dialogues et les erreurs |
| Édition | Annuler, rétablir, couper, copier, coller, supprimer et sélection globale apparaissent dans les ressources | Tester chaque commande selon la sélection et le mode actif |
| Graphique | Grille activable/affichable, propriétés de grille, zooms prédéfinis 50 à 150 %, ordre d'empilement et alignements | Relever les valeurs, l'accrochage et le résultat visuel |
| MCD | Dialogues dédiés aux entités, associations, rubriques, liens, héritages, CIF, contraintes inter-associations et règles | Inventorier toutes les conditions d'activation et transformations |
| Illustrations | Dialogues pour texte, rectangle, ellipse, flèche, ligne, crayon, image, vignette et liens entre illustrations | Tester création, rendu, connexion et sauvegarde |
| Rubriques | Catégories texte, numérique, date/heure et autre ; sous-types, taille, collation, précision, fuseau horaire, NOT NULL, UNIQUE, index et complément | Tester correspondances MLD/DDL et valeurs par défaut |
| Liens | Cardinalités 0,1 ; 1,1 ; 0,n ; 1,n, bornes libres, rôle, identifiant relatif, clé étrangère dans PK/index, cascades, lissage | Caractériser les cas binaires, réflexifs et n-aires |
| Héritage | Aucune contrainte, exclusivité, totalité, partition ; option de migration des rubriques ; SQL lié à l'héritage | Tester traduction MLD et DDL de chaque variante |
| Contraintes | Exclusivité, totalité, partition, simultanéité, inclusion ; CIF séparées ; SQL associé | Tester validation et génération SQL |
| Projections | Indices de MLD textuel, script LDD, Crow's Foot et vues | Vérifier les modes réellement accessibles et leur synchronisation |
| SQL | Configuration d'un SGBD personnalisé, types libres prédéfinis, options LDD, blocs SQL en début/fin de script et de création de table | Relever dialectes, substitutions et ordre d'insertion |
| Échanges | Ressources d'export graphique BMP, PNG, GIF et presse-papiers ; MLD en TXT/XML ; script LDD en TXT | Tester les formats effectifs et options de sortie |
| Rétroconception | Dialogue de saisie/import du script SQL ; dialogue de remplacement de caractères des noms de rubriques | Tester les syntaxes acceptées et le résultat créé |

Les 75 ressources de dialogue contiennent 42 dialogues numérotés entre 165 et
209, principalement liés au métier. Les 11 ressources de menu incluent aussi
des menus fournis par MFC. Les 2 tables d'accélérateurs comptent 21 entrées,
dont plusieurs raccourcis de l'éditeur, de fichiers et de l'aperçu avant
impression. Ce sont des nombres de ressources, pas des nombres de fonctionnalités.

## Corrections du catalogue

Les identifiants nouveaux ou enrichis dans `docs/FUNCTIONAL_CATALOG.md` couvrent
le format `.loo`, la grille, les types détaillés des rubriques, les SGBD
personnalisés, les blocs SQL complémentaires, les formats graphiques et les
options de rétroconception. Leur statut reste `À AUDITER/P0` jusqu'aux essais
boîte noire.

## Questions ouvertes prioritaires

1. Le ruban et les menus créés dynamiquement ne sont pas couverts par les seuls
   modèles `MENU` : il faut capturer chaque onglet et menu contextuel à l'écran.
2. Le format `.loo` doit être caractérisé avec plusieurs fichiers synthétiques,
   enregistrés après une seule modification chacun.
3. La liste exacte des SGBD intégrés et leurs réglages doit être relevée dans
   l'application.
4. Les composants de dialogue issus de MFC doivent être distingués de ceux
   effectivement exposés par Looping.
5. Les ressources 32 et 64 bits identiques n'impliquent pas des sorties
   identiques ; il faudra rejouer les mêmes modèles dans les deux éditions.

