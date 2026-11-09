# Contribuer à OpenLooping

Merci de contribuer à une réimplémentation indépendante destinée aux étudiants
et enseignants travaillant sous Linux.

## Principes indispensables

1. Ne jamais ajouter de code, ressource, icône ou texte extrait d'un logiciel
   propriétaire.
2. Séparer les observations reproductibles des hypothèses d'implémentation.
3. Ne pas ajouter au dépôt les exécutables de référence, captures privées,
   fichiers de cours nominatifs ou résultats bruts de rétro-ingénierie.
4. Ajouter un test pour toute correction de persistance, transformation SQL ou
   compatibilité de fichier.
5. Ne jamais annoncer une parité que le catalogue fonctionnel ne démontre pas.

## Construire et tester

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Structure d'une contribution

- une motivation pédagogique ou un scénario utilisateur clair ;
- l'origine de l'observation et son niveau de preuve ;
- une modification limitée et relisible ;
- des tests automatisés ;
- la mise à jour du catalogue, de la roadmap ou du journal des versions si la
  portée fonctionnelle change.

## Messages de commit

Le projet utilise des messages courts de type Conventional Commits :
`feat:`, `fix:`, `docs:`, `test:`, `build:`, `refactor:` ou `chore:`.
