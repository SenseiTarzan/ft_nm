//
// Created by gcaptari on 26/08/2026.
//
#include <stdio.h>
#include <stdlib.h>

/* 1. Symbole Absolu (A / a) */
/* Utilisé pour définir des constantes figées hors des sections mémoire */
__asm__(".global SYM_ABSOLU\n\t.set SYM_ABSOLU, 0xDEADC0DE");

/* 2. Données initialisées globales et locales (D / d) */
int variable_globale_initialisee = 42;                 /* D */
static int variable_locale_initialisee = 24;           /* d */

/* 3. Données non initialisées / BSS (B / b) */
int variable_globale_non_initialisee;                  /* B */
static int variable_locale_non_initialisee;            /* b */

/* 4. Données communes (C) */
/* Déclaré sans initialisation dans l'espace global (devient C avant d'être lié) */
int variable_commune;                                  /* C */

/* 5. Données en lecture seule (R / r) */
const int constante_globale = 100;                     /* R */
static const int constante_locale = 200;               /* r */
const char* chaine_rodata = "Une chaine dans .rodata"; /* R (pointeur) et r (contenu) */

/* 6. Variables par Thread / Thread-Local Storage (D / d ou B / b selon l'OS) */
__thread int tls_initialise = 777;                     /* Donnée de thread initialisée */
__thread int tls_non_initialise;                       /* Donnée de thread BSS */

/* 7. Symbole Faible (W / w) */
/* Peut être écrasé par une autre fonction du même nom au moment du lien */
__attribute__((weak)) void fonction_faible(void) {     /* W */
    printf("Fonction faible\n");
}

/* 8. Sections d'initialisation et de fermeture (.init_array / .fini_array) */
/* Ces attributs forcent l'écriture de pointeurs dans ces sections spécifiques */
__attribute__((constructor)) static void mon_constructeur(void) {
    /* Le pointeur va dans .init_array */
}

__attribute__((destructor)) static void mon_destructeur(void) {
    /* Le pointeur va dans .fini_array */
}

/* 9. Code exécutable / Text global et local (T / t) */
static void fonction_locale(void) {                     /* t */
    printf("Je suis une fonction statique locale.\n");
}

void fonction_globale(void) {                          /* T */
    printf("Je suis une fonction globale.\n");
    fonction_locale();
}

/* Point d'entrée standard */
int main(int argc, char** argv) {                      /* T */
    /* 10. Symbole non défini (U) */
    /* Provient d'une bibliothèque externe (la glibc ici) */
    printf("Hello World\n");                           /* printf sera marqué U */

    fonction_globale();
    fonction_faible();

    return 0;
}
