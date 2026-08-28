#ifndef ELF_STREAM_H
#define ELF_STREAM_H

#include <elf.h>
#include <stdio.h>

typedef struct s_elf_filestream t_elf_filestream;

typedef union {
    Elf64_Ehdr ehdr;
    Elf32_Ehdr ehdr_32;
} elf_header;

typedef struct s_elf_sym {
    union {
        Elf64_Sym sym;
        Elf32_Sym sym_32;
    };
    bool bit32;
} elf_sym;

typedef struct s_elf_section {
    union {
        Elf64_Shdr shdr;
        Elf32_Shdr shdr_32;
    };
    bool bit32;
} elf_section;

struct s_elf_filestream {
    char *filename;
    FILE *file;
    long int margin_position_elf;
    bool bit32;
    elf_header header;
};

typedef enum {
    ELF_OPEN_OK,
    ELF_OPEN_NOT_FOUND,
    ELF_OPEN_BAD_FORMAT,
} t_elf_open_status;

#define GET_E_SHNUM(stream) ((stream)->bit32 ? (stream)->header.ehdr_32.e_shnum : (stream)->header.ehdr.e_shnum)
#define GET_E_SHOFF(stream) ((stream)->bit32 ? (long int)(stream)->header.ehdr_32.e_shoff : (long int)(stream)->header.ehdr.e_shoff)
#define GET_E_SHENTSIZE(stream) ((stream)->bit32 ? (stream)->header.ehdr_32.e_shentsize : (stream)->header.ehdr.e_shentsize)
#define GET_E_SHSTRNDX(stream) ((stream)->bit32 ? (stream)->header.ehdr_32.e_shstrndx : (stream)->header.ehdr.e_shstrndx)

#define ELF_SHDR_INDEX(stream, index) (stream->margin_position_elf + GET_E_SHOFF(stream) + (GET_E_SHENTSIZE(stream) * index))

#define ELF_SYM_ST_TYPE(symbole) (symbole.bit32 ? ELF32_ST_TYPE(symbole.sym_32.st_info) : ELF64_ST_TYPE(symbole.sym.st_info))
#define ELF_SYM_ST_NAME(symbole) (symbole.bit32 ? symbole.sym_32.st_name : symbole.sym.st_name)
#define ELF_SYM_ST_SHNDX(symbole) (symbole.bit32 ? symbole.sym_32.st_shndx : symbole.sym.st_shndx)
#define ELF_SYM_ST_VALUE(symbole) (symbole.bit32 ? symbole.sym_32.st_value : symbole.sym.st_value)
#define ELF_SYM_ST_BIND(symbole) (symbole.bit32 ? ELF32_ST_BIND(symbole.sym_32.st_info) : ELF64_ST_BIND(symbole.sym.st_info))

#define ELF_SEC_SH_OFFSET(section) ((section.bit32 ? section.shdr_32.sh_offset : section.shdr.sh_offset))
#define ELF_SEC_SH_OFFSET_PTR(section) ( (section->bit32 ? section->shdr_32.sh_offset : section->shdr.sh_offset))
#define ELF_SEC_SH_TYPE(section) (section.bit32 ? section.shdr_32.sh_type : section.shdr.sh_type)
#define ELF_SEC_SH_FLAGS(section) (section.bit32 ? section.shdr_32.sh_flags : section.shdr.sh_flags)
#define ELF_SEC_SH_SIZE(section) (section.bit32 ? section.shdr_32.sh_size : section.shdr.sh_size)
#define ELF_SEC_SH_LINK(section) (section.bit32 ? section.shdr_32.sh_link : section.shdr.sh_link)

t_elf_open_status elf_filestream_open(char *filename, t_elf_filestream *stream, FILE *file);
t_elf_open_status elf_filestream_open_without_filename(t_elf_filestream *stream, FILE *file);
elf_section elf_filestream_get_section(t_elf_filestream *stream, unsigned int index);
bool elf_filestream_get_section_with_filter(elf_section *result, t_elf_filestream *stream, int n,
                                             bool (*filter)(const elf_section *section, bool bit32));
bool elf_filestream_get_symbol(elf_sym *result, t_elf_filestream *stream, elf_section *symtab, unsigned int index);
bool elf_filestream_has_section_beyond_eof(t_elf_filestream *stream);

#endif
