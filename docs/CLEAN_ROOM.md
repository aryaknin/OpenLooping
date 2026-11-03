# Protocole clean room

## Principe

Le logiciel est réécrit depuis une spécification comportementale indépendante.
Le dépôt ne doit contenir ni code, pseudo-code décompilé, ressource, icône, texte
créatif, ni binaire provenant du logiciel de référence.

## Chaîne de preuve

1. Créer une entrée synthétique ne contenant aucune donnée privée.
2. Observer Looping 4.1 en boîte noire.
3. Utiliser REA/Ghidra uniquement lorsque l'observation externe est insuffisante
   pour l'interopérabilité.
4. Consigner les entrées, actions, sorties, version et empreinte du binaire.
5. Rédiger une spécification indépendante sans pseudo-code décompilé.
6. Implémenter cette spécification dans une étape séparée.
7. Exécuter le même scénario sur la référence et la réimplémentation.

Les résultats bruts de REA/Ghidra restent hors du dépôt distribué. Toute fonction
du catalogue doit pointer vers une fiche d'observation et un test automatisé ou,
si l'automatisation est impossible, vers un protocole manuel reproductible.

## Niveaux de preuve

- `P0` : mention publique, non observée ;
- `P1` : interface et résultat observés une fois ;
- `P2` : cas nominal et cas limites documentés ;
- `P3` : test différentiel automatisé et reproductible.

