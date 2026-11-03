# Environnement d'analyse local

Les outils ci-dessous servent uniquement à établir les spécifications clean room.
Ils ne sont pas des dépendances de l'application C++/Qt et ne seront pas inclus
dans le paquet Debian.

## Versions validées

- REA `6.8.0` ;
- Ghidra `12.1.4` ;
- Eclipse Temurin JDK `21.0.13` ;
- Node.js portable `22.23.4` ;
- hôte Linux x86-64.

Tous sont installés sous `work/tools/`, chemin ignoré par Git. REA est enregistré
comme serveur MCP `rea` dans Codex avec le fournisseur `ghidra`. La configuration
Codex antérieure a été sauvegardée par l'installateur REA sous
`~/.codex/config.toml.rea.backup`.

Après une modification de cette configuration, Codex doit être redémarré pour
charger le nouveau serveur MCP dans une nouvelle session.

## Responsabilités

- **Ghidra** importe le PE Windows, désassemble, décompile et indexe fonctions,
  chaînes, appels et références. Ghidra est écrit en Java, d'où le JDK local.
- **REA** orchestre Ghidra et renvoie des observations structurées avec preuves,
  limites et empreintes.
- **OpenLooping** reste une application distincte écrite en C++20 et Qt 6. Java,
  Node, REA et Ghidra ne seront pas requis pour l'utiliser.

## Validation effectuée

Le diagnostic REA a validé Ghidra, son décompilateur Linux x86-64 et le JDK
complet. Une importation éphémère en lecture seule de `Looping.exe` s'est terminée
avec succès. L'exécutable d'origine n'a pas été modifié et le projet Ghidra
temporaire a été supprimé à la fermeture.

