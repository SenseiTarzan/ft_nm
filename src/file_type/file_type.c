//
// Created by gcaptari on 28/08/2026.
//

#include "file_type.h"

#include <elf.h>
#include <string.h>
#include <ar.h>

t_file_type detect_file_type(const char *filename , FILE **stream) {
    *stream = fopen(filename, "rb");
    if (!*stream) {
        *stream  = nullptr;
        return FILE_TYPE_ERROR;
    }
    if (ferror(*stream)) {
        fclose(*stream);
        *stream  = nullptr;
        return FILE_TYPE_ERROR;
    }
    long int pos = ftell(*stream);
    char buffer[SARMAG] = {0}; //21 3C 61 72 63 68 3E 0A
    if (fread(buffer, sizeof(buffer), 1, *stream) != 1) {
        fclose(*stream);
        *stream  = nullptr;
        return FILE_TYPE_ERROR;
    }
    if (memcmp(buffer, ARMAG, SARMAG) == 0) {
        return FILE_TYPE_STATIC_LIBRARIES;
    }
    memset(buffer, 0, sizeof(buffer));
    fseek(*stream, pos, SEEK_SET);
    if (fread(buffer, SELFMAG, 1, *stream) != 1) {
        fclose(*stream);
        *stream  = nullptr;
        return FILE_TYPE_ERROR;
    }
    if (memcmp(buffer, ELFMAG, SELFMAG) == 0) {
        fseek(*stream, pos, SEEK_SET);
        return FILE_TYPE_ELF;
    }
    fclose(*stream);
    *stream  = nullptr;
    return FILE_TYPE_UNKNOWN;
}
