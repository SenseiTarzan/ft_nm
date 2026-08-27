//
// Created by gcaptari on 26/08/2026.
//
#include <stdio.h>

/* =========================
 * Variables globales
 * ========================= */

/* .data */
int variable_globale = 42;

/* .bss */
int variable_bss;

/* .rodata */
const int constante_globale = 123;

/* =========================
 * Variables statiques
 * ========================= */

/* .data */
static int variable_locale = 10;

/* .bss */
static int variable_locale_bss;

/* .rodata */
static const int constante_locale = 456;

/* =========================
 * Fonctions
 * ========================= */

void fonction_globale(void)
{
    printf("fonction globale\n");
}

static void fonction_locale(void)
{
    printf("fonction locale\n");
}

/* =========================
 * Weak
 * ========================= */

void fonction_weak(void) __attribute__((weak));

void fonction_weak(void)
{
    printf("fonction weak\n");
}

/* =========================
 * TLS
 * ========================= */

__thread int variable_tls;

/* =========================
 * Main
 * ========================= */

int main(void)
{
    variable_bss = 1;
    variable_locale_bss = 2;
    variable_tls = 3;

    fonction_globale();
    fonction_locale();
    fonction_weak();

    printf("%d\n", variable_globale);
    printf("%d\n", constante_globale);

    return 0;
}