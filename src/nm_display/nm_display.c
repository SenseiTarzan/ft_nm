#include "nm_display.h"
#include <stdio.h>

static char determiner_type_nm(elf_sym sym, const elf_section sec) {
    const int bind = ELF_SYM_ST_BIND(sym);
    const int type = ELF_SYM_ST_TYPE(sym);
    char c = '?';

    /* Symboles weak */
    if (bind == STB_WEAK)
    {
        if (type == STT_OBJECT)
            return (ELF_SYM_ST_SHNDX(sym) == SHN_UNDEF) ? 'v' : 'V';

        return (ELF_SYM_ST_SHNDX(sym) == SHN_UNDEF) ? 'w' : 'W';
    }

    if (ELF_SYM_ST_SHNDX(sym) == SHN_UNDEF)
        return 'U';

    if (ELF_SYM_ST_SHNDX(sym) == SHN_ABS)
        c = 'A';

    else if (ELF_SYM_ST_SHNDX(sym) == SHN_COMMON)
        c = 'C';

    else
    {

        if (ELF_SEC_SH_TYPE(sec) == SHT_NOBITS && (ELF_SEC_SH_FLAGS(sec) & SHF_ALLOC))
            c = 'B';
        else if (ELF_SEC_SH_FLAGS(sec) & SHF_EXECINSTR)
            c = 'T';
        else if (ELF_SEC_SH_FLAGS(sec) & SHF_WRITE)
            c = 'D';
        else if (ELF_SEC_SH_FLAGS(sec) & SHF_ALLOC)
            c = 'R';
        else if (ELF_SEC_SH_TYPE(sec) == SHT_PROGBITS)
            c = 'N';
    }

    if (bind == STB_LOCAL && c >= 'A' && c <= 'Z')
        c += ('a' - 'A');

    return c;
}

void display_symbols(const struct s_symbole_elf *symboles, uint64_t count, bool bit32) {
    for (uint64_t i = 0; i < count; i++) {
        char type_lettre = determiner_type_nm(symboles[i].symbole, symboles[i].section);

        if (bit32) {
            if (ELF_SYM_ST_SHNDX(symboles[i].symbole) == SHN_UNDEF) {
                printf("         %c %s\n", type_lettre, symboles[i].name);
            } else {
                printf("%08lx %c %s\n", ELF_SYM_ST_VALUE(symboles[i].symbole), type_lettre, symboles[i].name);
            }
        } else {
            if (ELF_SYM_ST_SHNDX(symboles[i].symbole) == SHN_UNDEF) {
                printf("                 %c %s\n", type_lettre, symboles[i].name);
            } else {
                printf("%016lx %c %s\n", ELF_SYM_ST_VALUE(symboles[i].symbole), type_lettre, symboles[i].name);
            }
        }
    }
}
