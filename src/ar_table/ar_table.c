#include "ar_table.h"

#include <stdlib.h>
#include <string.h>

static void ar_name_push_back(t_ar_name **head, struct s_ar_name *node) {
    if (*head == nullptr) {
        *head = node;
        return;
    }
    struct s_ar_name *move = *head;
    while (move->next != nullptr) {
        move = move->next;
    }
    move->next = node;
}

static struct s_ar_name *new_ar_name(char *name, long int index) {
    struct s_ar_name *node = malloc(sizeof(struct s_ar_name));
    if (!node) {
        return nullptr;
    }
    node->name = name;
    node->index = index;
    node->next = nullptr;
    return node;
}


void ar_name_free(struct s_ar_name **head) {
    if (head == nullptr) {
        return;
    }
    struct s_ar_name *move = *head;
    while (move != nullptr) {
        struct s_ar_name *tmp = move;
        free(move->name);
        move->name = nullptr;
        move = move->next;
        free(tmp);
    }
    *head = nullptr;
}


static char* ar_name_get(t_ar_name *head, long int index) {
    if (head == nullptr) {
        return nullptr;
    }
    const struct s_ar_name *move = head;
    while (move != nullptr) {
        if (move->index == index) {
            return move->name;
        }
        move = move->next;
    }
    return nullptr;
}

static void ar_node_push_back(struct s_ar_node **head, struct s_ar_node *node) {
    if (*head == NULL) {
        *head = node;
        return;
    }
    struct s_ar_node *tmp = *head;
    while (tmp->next != nullptr) {
        tmp = tmp->next;
    }
    tmp->next = node;
}

void ar_node_free(struct s_ar_node **head) {
    if (head == nullptr) {
        return;
    }
    struct s_ar_node *move = *head;
    while (move != nullptr) {
        struct s_ar_node *tmp = move;
        free_elf_symbols(move->symbole, move->count);
        move->symbole = nullptr;
        move->count = 0;
        if (move->name_heap) {
            free(move->name);
            move->name = nullptr;
        }
        move = move->next;
        free(tmp);
    }
    *head = nullptr;
}



// Helper function to safely extract and null-terminate fixed-width ASCII fields
void safe_extract(char *dest, const char *src, size_t size) {
    memcpy(dest, src, size);
    dest[size] = '\0';

    // Trim trailing spaces common in ar headers
    int i = size - 1;
    while (i >= 0 && (dest[i] == ' ' || dest[i] == '/')) {
        dest[i] = '\0';
        i--;
    }
}

#define LONG_NAMES_NAME "//              "

bool parse_ar_symbols(FILE *file, t_ar_node **out_ar_headers,t_ar_name **ar_names, uint64_t *out_count) {
    t_elf_filestream stream;
    struct ar_hdr hdr;
    long int save_pos = ftell(file);
    while (fread(&hdr, sizeof(struct ar_hdr), 1, file) == 1) {
        // Validate header trailer magic characters (`\n)
        if (memcmp(hdr.ar_fmag, ARFMAG, 2) != 0) {
            break;
        }

        const long size = strtol(hdr.ar_size, nullptr, 10);
        if (memcmp (hdr.ar_name, LONG_NAMES_NAME, 16) == 0) {
            long int i = 0;
            while (i < size) {
                size_t name_len = 0;
                char c;
                const long int tmp_pos = ftell(file);
                bool readable = fread(&c, 1, 1, file) == 1;
                while ((i + (long int) name_len) < size && readable && c != '/') {
                    name_len++;
                    readable = fread(&c, 1, 1, file) == 1;
                }

                fseek(file, tmp_pos, SEEK_SET);
                if (!readable) {
                    ar_name_free(ar_names);
                    break;
                } else if (name_len == 0) {
                    ar_name_free(ar_names);
                    break;
                } else {
                    char *name = malloc(name_len + 1);
                    if (!name) {
                        continue;
                    }
                    fread(name, 1, name_len, file);
                    name[name_len] = '\0';
                    struct s_ar_name *node_name = new_ar_name(name, i);
                    if (node_name == nullptr) {
                        free(name);
                        continue;
                    }
                    ar_name_push_back(ar_names, node_name);
                }
                i += (long int) name_len + 2;
                fseek(file,  2, SEEK_CUR);
            }
        }
        fseek(file, (size + 1) & ~1, SEEK_CUR);

    }
    fseek(file, save_pos, SEEK_SET);

    // 2. Loop through all file members
    while (fread(&hdr, sizeof(struct ar_hdr), 1, file) == 1) {
        // Validate header trailer magic characters (`\n)
        if (memcmp(hdr.ar_fmag, ARFMAG, 2) != 0) {
            break;
        }

        // Allocate temporary stack buffers (+1 for null-terminator)
        char name[17], size_str[11];
        safe_extract(size_str, hdr.ar_size, 10);
        struct s_ar_node *ar_node = calloc(1, sizeof(struct s_ar_node));
        if (!ar_node) {
            continue;
        }
        safe_extract(name, hdr.ar_name, 16);
        const long file_size = strtol(size_str, nullptr, 10);
        // Ignorer les en-têtes système d'indexation ("/" ou "//")
        if (strcmp(name, "/") == 0 || strcmp(name, "//") == 0) {
            fseek(file, (file_size + 1) & ~1, SEEK_CUR);
            continue;
        }
        if (strlen(name) == 0 || strcmp(name, "__.SYMDEF") == 0) {
            const long padding = file_size % 2;
            fseek(file, file_size + padding, SEEK_CUR);
            ar_node_free(&ar_node);
            continue;
        }
        if (name[0] == '/' && name[1] >= '0' && name[1] <= '9') {
            const long int index = strtol(name + 1, NULL, 10);
            char *real_name = ar_name_get(*ar_names,index);
            if (real_name == nullptr) {
                ar_node->name = "<corrupt>";
                ar_node->name_heap = false;
            }else {
                ar_node->name = real_name;
                ar_node->name_heap = false;
            }
        }else {
            ar_node->name = strdup(name);
            ar_node->name_heap = true;
            if (ar_node->name == nullptr) {
                free(ar_node->name);
                ar_node_free(&ar_node);
                break;
            }
        }
        const long padding = file_size % 2;
        const long data_start_pos = ftell(file);
        stream.margin_position_elf = data_start_pos;
        switch (elf_filestream_open_without_filename(&stream, file)) {
            case ELF_OPEN_OK:
                (*out_count)++;
                break;
            default:
                ar_node_free(&ar_node);
                fseek(file, data_start_pos + file_size + padding, SEEK_SET);
                continue;
        }
        if (!parse_elf_symbols(&stream, &ar_node->symbole, &ar_node->count) || ar_node->count == 0) {
            ar_node_free(&ar_node);
            fseek(file, data_start_pos + file_size + padding, SEEK_SET);
            continue;
        }
        ar_node->bit32 = stream.bit32;
        ar_node_push_back(out_ar_headers, ar_node);
        fseek(file, data_start_pos + file_size + padding, SEEK_SET);
    }
    return true;
}

void ar_node_foreach(t_ar_node *head, void (*func)(t_ar_node *)) {
    if (head == nullptr) {
        return;
    }
    struct s_ar_node *move = head;
    while (move != NULL) {
        func(move);
        move = move->next;
    }
}

