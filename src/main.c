#include "elf_stream.h"
#include "symbol_table.h"
#include "nm_display.h"
#include <stdio.h>
#include <locale.h>

static void read_elf(t_elf_filestream *stream, const char *filename) {
    struct s_symbole_elf *symboles;
    uint64_t count;

    if (!parse_elf_symbols(stream, &symboles, &count) || count == 0) {
        fprintf(stderr, "ft_nm: %s: no symbols\n", filename);
        return;
    }

    display_symbols(symboles, count, stream->bit32);
    free_symbols(symboles, count);
}


int main(int argc, char **argv) {
    setlocale(LC_COLLATE, "");
    const char *filename = argc > 1 ? argv[1] : "a.out";
    t_elf_filestream stream;

    switch (elf_filestream_open((char *)filename, &stream)) {
        case ELF_OPEN_NOT_FOUND:
            fprintf(stderr, "ft_nm: '%s': No such file or directory\n", filename);
            return 1;
        case ELF_OPEN_BAD_FORMAT:
            fprintf(stderr, "ft_nm: %s: file format not recognized\n", filename);
            return 1;
        case ELF_OPEN_OK:
            break;
    }

    if (elf_filestream_has_section_beyond_eof(&stream)) {
        fprintf(stderr, "ft_nm: attention: %s a une section qui s'étend au delà de la fin du fichier\n", filename);
    }

    read_elf(&stream, filename);
    fclose(stream.file);
    return 0;

}
