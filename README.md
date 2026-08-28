# ft_nm

Réimplémentation en C de la commande Unix `nm`, qui liste les symboles
contenus dans un fichier objet ou exécutable au format ELF (32 et 64 bits).

## Fonctionnement

Le programme lit et parse le format ELF directement (headers, sections,
table des symboles, table des chaînes) pour afficher, pour chaque symbole :
adresse, type (code, données, undefined, ...) et nom, comme le ferait `nm`.

Les bibliothèques statiques Linux (`.a`, archives `ar`) sont également
supportées : `ft_nm` parcourt les fichiers objets contenus dans l'archive
et affiche les symboles de chacun, précédés du nom du fichier objet
correspondant.

Les bibliothèques dynamiques Linux (`.so`) sont également supportées,
`ft_nm` lit leur table des symboles dynamiques (ainsi que la table des
symboles classique si présente) comme pour un exécutable ELF.

## Structure

- `src/elf_stream/` — lecture et parsing bas niveau du fichier ELF
- `src/symbol_table/` — extraction de la table des symboles
- `src/nm_display/` — formatage et affichage des symboles
- `src/main.c` — point d'entrée
- `test/` — programmes et binaires de test

## Compilation

Avec `make` :

```sh
make
```

Ou avec CMake :

```sh
cmake -B build
cmake --build build
```

## Utilisation

```sh
./ft_nm <fichier>
```

Sans argument, `ft_nm` utilise `a.out` par défaut.

Le fichier peut être un objet ELF, un exécutable ELF, une bibliothèque
statique (`.a`) ou une bibliothèque dynamique (`.so`).

## Usage de l'IA

L'assistance IA sur ce projet s'est limitée à la génération des messages
de commit et de ce README. Le code source a été écrit sans IA.