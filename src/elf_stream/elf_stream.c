#include "elf_stream.h"
#include <string.h>

static bool elf_filestream_read_header(t_elf_filestream *stream) {
    rewind(stream->file);
    if (stream->bit32) {
        return fread(&stream->header.ehdr_32, sizeof(Elf32_Ehdr), 1, stream->file) == 1;
    }
    return fread(&stream->header.ehdr, sizeof(Elf64_Ehdr), 1, stream->file) == 1;
}

t_elf_open_status elf_filestream_open(char *filename, t_elf_filestream *stream) {
    stream->filename = filename;
    stream->file = fopen(filename, "rb");
    if (!stream->file) {
        return ELF_OPEN_NOT_FOUND;
    }
    unsigned char e_ident[6];
    if (fread(e_ident, 1, 6, stream->file) != 6 || memcmp(e_ident, ELFMAG, SELFMAG) != 0) {
        fclose(stream->file);
        stream->file = nullptr;
        return ELF_OPEN_BAD_FORMAT;
    }
    stream->bit32 = (e_ident[4] == ELFCLASS32);
    if (!elf_filestream_read_header(stream)) {
        fclose(stream->file);
        stream->file = nullptr;
        return ELF_OPEN_BAD_FORMAT;
    }
    rewind(stream->file);
    return ELF_OPEN_OK;
}

elf_section elf_filestream_get_section(t_elf_filestream *stream, unsigned int index) {
    elf_section section = {0};
    if (fseek(stream->file, ELF_SHDR_INDEX(stream, index), SEEK_SET) != 0) {
        return section;
    }
    if (stream->bit32) {
        if (fread(&section.shdr_32, GET_E_SHENTSIZE(stream), 1, stream->file) != 1) {
            return (elf_section){0};
        }
        section.bit32 = true;
    } else {
        if (fread(&section.shdr, GET_E_SHENTSIZE(stream), 1, stream->file) != 1) {
            return (elf_section){0};
        }
        section.bit32 = false;
    }
    return section;
}

bool elf_filestream_get_section_with_filter(elf_section *result, t_elf_filestream *stream, int n,
                                             bool (*filter)(const elf_section *section, bool bit32)) {
    const bool bit32 = stream->bit32;
    bool found = false;
    for (int i = 0; i < n; i++) {
        const elf_section section = elf_filestream_get_section(stream, i);
        if (filter(&section, bit32)) {
            *result = section;
            found = true;
        }
    }

    return found;
}

bool elf_filestream_get_symbol(elf_sym *result, t_elf_filestream *stream, elf_section *symtab, unsigned int index) {
    elf_sym sym = {0};
    size_t sym_size = stream->bit32 ? sizeof(Elf32_Sym) : sizeof(Elf64_Sym);
    if (fseek(stream->file, ELF_SEC_SH_OFFSET_PTR(symtab) + (long)(sym_size * index), SEEK_SET) != 0) {
        return false;
    }
    if (stream->bit32) {
        if (fread(&sym.sym_32, sizeof(Elf32_Sym), 1, stream->file) != 1) {
            return false;
        }
        sym.bit32 = true;
    } else {
        if (fread(&sym.sym, sizeof(Elf64_Sym), 1, stream->file) != 1) {
            return false;
        }
        sym.bit32 = false;
    }
    *result = sym;
    return true;
}

bool elf_filestream_has_section_beyond_eof(t_elf_filestream *stream) {
    long saved_pos = ftell(stream->file);
    fseek(stream->file, 0, SEEK_END);
    long file_size = ftell(stream->file);
    fseek(stream->file, saved_pos, SEEK_SET);

    bool beyond_eof = false;
    unsigned short int n = GET_E_SHNUM(stream);
    for (unsigned short int i = 0; i < n; i++) {
        elf_section section = elf_filestream_get_section(stream, i);
        if (ELF_SEC_SH_TYPE(section) == SHT_NOBITS) {
            continue;
        }
        unsigned long sh_offset = ELF_SEC_SH_OFFSET(section);
        unsigned long sh_size = ELF_SEC_SH_SIZE(section);
        if ((long)(sh_offset + sh_size) > file_size) {
            beyond_eof = true;
            break;
        }
    }
    return beyond_eof;
}
