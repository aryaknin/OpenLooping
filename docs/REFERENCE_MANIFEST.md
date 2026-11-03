# Manifeste de la référence Looping 4.1

Provenance déclarée : archive téléchargée par l'utilisateur depuis le site
officiel de Looping. Les fichiers restent sous `work/reference/` et sont exclus
du dépôt.

| Fichier | Taille | SHA-256 |
|---|---:|---|
| `Looping (1).zip` | 7 120 361 | `027834def4c01798de9dea0c328defe6dc1c0317e13b0bc796e495ab8e68c404` |
| `Looping.exe` | 8 113 152 | `3ba421572967b02b7fa7962ada6e85fc96cc6b68f697bf4bb6e5db61dabe3631` |
| `Looping_32bits.exe` | 7 225 856 | `9d12bfe357a866f41ef2f6e993a39d990e4732dcad3fac83d12cae2250420f30` |

## Première observation statique du binaire 64 bits

- format PE Windows x86-64 ;
- base d'image `0x140000000` ;
- 8 segments mémoire recensés ;
- 13 538 procédures définies par l'analyse Ghidra ;
- 4 832 chaînes définies par l'analyse Ghidra ;
- référence à un PDB nommé `looping.pdb`, non fourni ;
- analyse Ghidra 12.1.4 terminée avant interrogation REA.

Ces nombres décrivent l'inventaire établi par Ghidra, pas une preuve que chaque
fonction ou chaîne du programme a été reconnue. Ils ne constituent pas une
spécification fonctionnelle et aucun pseudo-code n'est conservé dans ce dépôt.

