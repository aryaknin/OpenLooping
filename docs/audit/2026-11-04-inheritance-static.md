# Audit 05 — ressources statiques de l’héritage

Date : 4 novembre 2026. Cible : `Looping_32bits.exe` 4.1, SHA-256
`9d12bfe357a866f41ef2f6e993a39d990e4732dcad3fac83d12cae2250420f30`.

Ghidra 12.1.4, piloté par REA, confirme la chaîne `Partition` dans les données
définies du binaire (Evidence
`ev_1c81664b1d3345bb1c2368f2f43494dcc835576950c8a7387ed7239aa3249de5`).
Cette recherche statique ne prouve aucun comportement d’exécution.

Le décodage en lecture seule des ressources PE identifie le dialogue 174,
« Héritage ». Il contient : Nom ; Pas de contrainte ; Exclusivité ; Totalité ;
Partition ; migration de toutes les rubriques du surtype vers les sous-types ;
Commentaire ; Script SQL associé à l’héritage.

Les dialogues 175, 197, 198 et 199 sont titrés « Lien héritage ». Les champs
communs observés comprennent : Surtype de l’héritage ; expression du critère de
spécialisation/généralisation ; Afficher ; Suffixer clés étrangères ; Renommer
clé étrangère ; position dans la clé primaire ; compléments `ON DELETE CASCADE`,
`ON UPDATE CASCADE` et libre ; Lissage cassure. Certaines variantes ajoutent la
construction d’index composé.

## Limites

- Les valeurs par défaut et conditions d’activation ne sont pas établies par
  les ressources statiques.
- La forme graphique exacte, le protocole de connexion et les transformations
  MLD/SQL ne sont pas prouvés.
- L’implémentation peut donc conserver et éditer ces propriétés, mais ne doit
  pas prétendre reproduire la stratégie relationnelle avant un audit dynamique.
