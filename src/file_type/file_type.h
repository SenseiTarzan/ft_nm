//
// Created by gcaptari on 28/08/2026.
//

#ifndef FT_NM_FILE_TYPE_H
#define FT_NM_FILE_TYPE_H

#include <stdio.h>
typedef enum {
    FILE_TYPE_ERROR,
    FILE_TYPE_UNKNOWN = -1,
    FILE_TYPE_ELF,
    FILE_TYPE_STATIC_LIBRARIES
} t_file_type;

t_file_type detect_file_type(const char *filename , FILE **stream);

#endif //FT_NM_FILE_TYPE_H
