# Carte de couverture des dialogues embarqués

Référence : Looping 4.1, ressources `RT_DIALOG` décodées en lecture seule depuis
les exécutables répertoriés dans `docs/REFERENCE_MANIFEST.md`. Les deux
éditions exposent les mêmes modèles de dialogue. Une ressource peut être
inactive ou relever du framework MFC : la colonne « essai » reste obligatoire.

| ID ressource | Objet ou réglage apparent | Exigences liées | Essai boîte noire |
|---:|---|---|---|
| 165 | Entité, rubriques, entité fictive/externe, génération MLD | MCD-001/002/011/012 | À faire |
| 166 | Zoom personnalisé | CAN-002 | À faire |
| 167 | Association, rubriques et table de correspondance | MCD-005/006, MAP-004 | À faire |
| 170 | Dimensions du projet et géométrie par défaut | CAN-005/012 | À faire |
| 171 | Texte, police, alignement, contour, fond, ombre | MCD-021, CAN-012 | À faire |
| 172 | Rubrique, types, taille, collation, précision, clés et index | MCD-002/004/013/025/026 | À faire |
| 173 | Lien principal, cardinalité, rôle, clés, cascade | MCD-007/008/009, SQL-009 | Partiel : fiche et 0,1/1,1/0,n |
| 174 | Héritage, contraintes, migration et SQL | MCD-014/015, MAP-010/011 | À faire |
| 175 | Lien d'héritage, critère, clés et cascade | MCD-014, MAP-010, SQL-009 | À faire |
| 176 | Contrainte inter-associations et SQL | MCD-019/020 | À faire |
| 177 | Lien d'une contrainte inter-associations | MCD-019 | À faire |
| 178 | Lien d'une CIF | MCD-016/018 | À faire |
| 179 | CIF | MCD-016/017/018 | À faire |
| 180 | Règle, description, affichage et position du SQL | MCD-020, SQL-012 | À faire |
| 181 | Lien d'une règle | MCD-020 | À faire |
| 182 | Rectangle | MCD-027 | À faire |
| 183 | Ellipse | MCD-027 | À faire |
| 184 | Flèche | MCD-027 | À faire |
| 185 | Ligne | MCD-027 | À faire |
| 186 | Lien entre illustrations | MCD-022/027 | À faire |
| 187 | SGBD personnalisé et types associés | SQL-014 | À faire |
| 189 | Types libres prédéfinis | SQL-014 | À faire |
| 190 | Compléments SQL au niveau script/table | SQL-015 | À faire |
| 191 | Crayon | MCD-027 | À faire |
| 192 | Message avec option de ne plus afficher | APP-012 | À faire |
| 193 | Recherche de texte | APP-014 | À faire |
| 194–196 | Variantes de la fenêtre de lien | MCD-007/008/009, SQL-009 | À faire |
| 197–199 | Variantes de la fenêtre de lien d'héritage | MCD-014, MAP-010 | À faire |
| 200 | Message proposant corriger ou ignorer | MCD-024 | À faire |
| 201 | Urbanisation et gestion d'une liste | VIEW-009/010 | À faire |
| 202 | Vue nommée | VIEW-009 | À faire |
| 203 | Image | MCD-027 | À faire |
| 204 | Vignette liée à un modèle | VIEW-010, MCD-027 | À faire |
| 205 | Couleurs | CAN-011 | À faire |
| 206 | Rétroconception à partir de SQL | REV-001/002 | À faire |
| 207 | Traitement sans contrôles statiques | REV-001/011 | À faire |
| 208 | Remplacement de caractères dans les rubriques | REV-012 | À faire |
| 209 | Dialogue « urbanisme » sans contrôles statiques | VIEW-009/010 | À faire |

## Ressources hors de cette séquence

Les dialogues 100, 102, 123, 131, 143, 145, 151, 163 et 164 devront être
classés par observation : certains montrent des termes génériques ou anciens
sans lien certain avec l'interface active. Les dialogues à identifiant élevé
(16 000 et plus) semblent majoritairement fournis par MFC, mais leur éventuelle
présence dans les menus de personnalisation doit être vérifiée.

## Méthode d'essai

Pour chaque ligne : noter le chemin d'accès, les préconditions, les valeurs par
défaut, les contrôles visibles et cachés, les messages de validation, l'effet
sur le canevas, le fichier `.loo`, le MLD et le DDL, puis exécuter au moins un
cas nominal et un cas limite. L'observation doit devenir une fiche indépendante
avant l'implémentation C++.

