# Audit 06 — contraintes, CIF et règles (ressources statiques)

Date : 4 novembre 2026. Cible : `Looping_32bits.exe` 4.1, SHA-256
`9d12bfe357a866f41ef2f6e993a39d990e4732dcad3fac83d12cae2250420f30`.

REA a inventorié intégralement 1 048 ressources PE parmi 2 113 entrées
examinées (Evidence
`ev_cc1e15d6f8c01ba69941372edf08a65217a9fa56b92561aa78a116f046f1e9e3`).
Les charges des dialogues restent opaques pour cet inventaire REA ; elles ont
été décodées séparément, en lecture seule, par le parseur de ressources du
laboratoire clean-room.

Le dialogue 176, « Contrainte inter-associations », expose les types
Exclusivité, Totalité, Partition, Simultanéité et Inclusion, plus commentaire et
script SQL. Le dialogue 177 décrit son lien vers l’association ciblée, son
libellé et le lissage de cassure.

Les dialogues 178 et 179 décrivent respectivement le lien d’une CIF vers
l’entité ciblée et la CIF elle-même (nom et commentaire).

Le dialogue 180, « Règle », expose : nom, description, affichage dans le modèle,
ajout de la description comme commentaire SQL, script SQL, position avant ou
après la création des tables, et affichage du SQL dans le modèle. Le dialogue
181 ajoute un lien de règle avec libellé et lissage de cassure.

Les ressources prouvent les champs, mais ni leurs valeurs par défaut ni le
protocole graphique de liaison. La version 0.4.3 implémente donc la règle
autonome et son ordre SQL ; les liens, CIF et contraintes restent à confirmer.
