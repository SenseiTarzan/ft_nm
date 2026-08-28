#ifndef AR_TABLE_H
#define AR_TABLE_H

#include "elf_stream.h"
#include <stdint.h>
#include <ar.h>
#include "../symbol_table/symbol_table.h"

typedef struct s_ar_node {
    char *name;
    bool name_heap;
    struct s_symbole_elf *symbole;
    uint64_t count;
    bool bit32;
    struct s_ar_node *next;
} t_ar_node;

typedef struct s_ar_name {
    char *name;
    long int index;
    struct s_ar_name *next;
} t_ar_name;

bool parse_ar_symbols(FILE *file, t_ar_node **out_ar_headers, t_ar_name **ar_names, uint64_t *out_count);
void ar_node_free(t_ar_node **head);

void ar_node_foreach(t_ar_node *head, void (*func)(t_ar_node *));

void ar_name_free(struct s_ar_name **head);

#endif
