#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>

typedef struct s_symbole_node {
    struct s_symbole_elf data;
    struct s_symbole_node *next;
} t_symbole_node;

static bool section_is_symtab(const elf_section *section, bool bit32) {
    if (bit32) {
        return section->shdr_32.sh_type == SHT_SYMTAB;
    }
    return section->shdr.sh_type == SHT_SYMTAB;
}

static int compare_symboles_nm(const void *a, const void *b)
{
    const struct s_symbole_elf *x = a;
    const struct s_symbole_elf *y = b;
    const char *xn = x->name;
    const char *yn = y->name;

    if (yn == nullptr)
        return xn != nullptr;
    if (xn == nullptr)
        return -1;

    if (*yn == '\0')
        return *xn != '\0';
    if (*xn == '\0')
        return -1;

    return strcoll(xn, yn);
}

static char *make_corrupt_placeholder(void) {
    static const char placeholder[] = "<corrupt>";
    char *name = malloc(sizeof(placeholder));
    if (name) {
        memcpy(name, placeholder, sizeof(placeholder));
    }
    return name;
}

bool parse_elf_symbols(t_elf_filestream *stream, struct s_symbole_elf **out_symboles, uint64_t *out_count) {
    elf_section symtab_hdr;
    if (!elf_filestream_get_section_with_filter(&symtab_hdr, stream, GET_E_SHNUM(stream), &section_is_symtab)) {
        return false;
    }
    unsigned long strtab_index = ELF_SEC_SH_LINK(symtab_hdr);
    elf_section strtab_hdr = elf_filestream_get_section(stream, strtab_index);

    uint64_t nb_symboles = ELF_SEC_SH_SIZE(symtab_hdr) / (stream->bit32 ? sizeof(Elf32_Sym) : sizeof(Elf64_Sym));
    t_symbole_node *list = nullptr;
    uint64_t count = 0;

    for (uint64_t i = 0; i < nb_symboles; i++) {
        elf_sym test_sym;
        if (!elf_filestream_get_symbol(&test_sym, stream, &symtab_hdr, i)) {
            continue;
        }
        if (ELF_SYM_ST_TYPE(test_sym) == STT_FILE) {
            continue;
        }

        long name_offset = stream->margin_position_elf +   (long)ELF_SEC_SH_OFFSET(strtab_hdr) + ELF_SYM_ST_NAME(test_sym);
        size_t name_len = 0;
        char c;

        fseek(stream->file, name_offset, SEEK_SET);
        bool readable = fread(&c, 1, 1, stream->file) == 1;
        while (readable && c != 0) {
            name_len++;
            readable = fread(&c, 1, 1, stream->file) == 1;
        }

        char *name;
        if (!readable) {
            name = make_corrupt_placeholder();
            if (!name) {
                continue;
            }
            fprintf(stderr, "ft_nm: %s: tentative de charger des chaînes depuis une section non-chaînes (numéro %lu)\n",
                    stream->filename, strtab_index);
        } else if (name_len == 0) {
            continue;
        } else {
            name = malloc(name_len + 1);
            if (!name) {
                continue;
            }
            fseek(stream->file, name_offset, SEEK_SET);
            fread(name, 1, name_len, stream->file);
            name[name_len] = '\0';
        }

        elf_section shdr = elf_filestream_get_section(stream, ELF_SYM_ST_SHNDX(test_sym));

        t_symbole_node *node = malloc(sizeof(t_symbole_node));
        if (!node) {
            free(name);
            continue;
        }
        node->data.symbole = test_sym;
        node->data.section = shdr;
        node->data.name = name;
        node->next = list;
        list = node;
        count++;
    }

    struct s_symbole_elf *symboles = nullptr;
    if (count > 0) {
        symboles = malloc(count * sizeof(struct s_symbole_elf));
        if (!symboles) {
            while (list) {
                t_symbole_node *next = list->next;
                free(list->data.name);
                free(list);
                list = next;
            }
            return false;
        }
    }

    uint64_t idx = 0;
    while (list) {
        t_symbole_node *next = list->next;
        symboles[idx++] = list->data;
        free(list);
        list = next;
    }

    qsort(symboles, count, sizeof(struct s_symbole_elf), compare_symboles_nm);

    *out_symboles = symboles;
    *out_count = count;
    return true;
}

void free_elf_symbols(struct s_symbole_elf *symboles, uint64_t count) {
    if (symboles == nullptr) {
        return;
    }
    for (uint64_t i = 0; i < count; i++) {
        free(symboles[i].name);
    }
    free(symboles);
}
