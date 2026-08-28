#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "elf_stream.h"
#include <stdint.h>

struct s_symbole_elf {
    elf_sym symbole;
    char *name;
    elf_section section;
};


bool parse_elf_symbols(t_elf_filestream *stream, struct s_symbole_elf **out_symboles, uint64_t *out_count);
void free_elf_symbols(struct s_symbole_elf *symboles, uint64_t count);

#endif
