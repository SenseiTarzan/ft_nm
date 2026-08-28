
#include "elf_stream.h"
#include "nm_display.h"
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "file_type/file_type.h"
#include "ar_table/ar_table.h"
#include <ar.h>


static void read_elf(t_elf_filestream *stream) {
    struct s_symbole_elf *symboles;
    uint64_t count;

    if (!parse_elf_symbols(stream, &symboles, &count) || count == 0) {
        fprintf(stderr, "ft_nm: %s: no symbols\n", stream->filename);
        return;
    }

    display_symbols(symboles, count, stream->bit32);
    free_elf_symbols(symboles, count);
}

static void display_ar_table(t_ar_node *node) {
    printf("%s:\n", node->name);
    display_symbols(node->symbole, node->count, node->bit32);
}

static void read_ar(FILE *file, const char *filename) {
    t_ar_node *node = nullptr;
    t_ar_name *ar_name = nullptr;
    uint64_t count = 0;
    if (!parse_ar_symbols(file, &node, &ar_name, &count) || count == 0) {
        fprintf(stderr, "ft_nm: %s: no symbols\n", filename);
        ar_name_free(&ar_name);
        ar_node_free(&node);
        return;
    }
    ar_node_foreach(node, display_ar_table);
    ar_name_free(&ar_name);
    ar_node_free(&node);
}



int main(int argc, char **argv) {
    setlocale(LC_COLLATE, "");
    printf("argc %i\n", argc);
    long int index = 1;
    while (index < argc) {
        FILE* file = nullptr;
        const char* filename = argv[index];
        printf("%s:\n", filename);
        switch (detect_file_type(filename, &file)) {
            case FILE_TYPE_ELF: {
                t_elf_filestream stream;
                stream.margin_position_elf = 0;
                switch (elf_filestream_open((char *)filename, &stream, file)) {
                    case ELF_OPEN_NOT_FOUND:
                        fprintf(stderr, "ft_nm: '%s': No such file or directory\n", filename);
                        break;
                    case ELF_OPEN_BAD_FORMAT:
                        fprintf(stderr, "ft_nm: %s: file format not recognized\n", filename);
                        break;
                    case ELF_OPEN_OK:
                        if (elf_filestream_has_section_beyond_eof(&stream)) {
                            fprintf(stderr, "ft_nm: attention: %s a une section qui s'étend au delà de la fin du fichier\n", filename);
                        }
                        read_elf(&stream);
                        break;
                }
                break;
            }

            case FILE_TYPE_STATIC_LIBRARIES: {
                read_ar(file, filename);
                break;
            }
            default:
                break;

        }
        if (file != nullptr) {
            fclose(file);
        }
        index++;
    }

    return 0;

}
