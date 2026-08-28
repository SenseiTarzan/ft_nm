cmake_minimum_required(VERSION 3.25)
project(ft_nm C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror)

include_directories(
        src
        src/elf_stream
        src/file_type
        src/symbol_table
        src/nm_display
        src/ar_table
)

add_executable(ft_nm
        src/main.c
        src/elf_stream/elf_stream.c
        src/elf_stream/elf_stream.h
        src/symbol_table/symbol_table.c
        src/symbol_table/symbol_table.h
        src/nm_display/nm_display.c
        src/nm_display/nm_display.h
        src/file_type/file_type.c
        src/ar_table/ar_table.c
)
