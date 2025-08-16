# chibicc-riscv C compiler
set(CHIBICC_SOURCES
    main.c
    parse.c
    codegen.c
    preprocess.c
    strings.c
    tokenize.c
    type.c
)

add_executable(irre-cc ${CHIBICC_SOURCES})

# Set C11 standard and compiler flags matching original Makefile
target_compile_features(irre-cc PRIVATE c_std_11)
target_compile_options(irre-cc PRIVATE -g -fno-common)

# Include directories for chibicc headers 
# Note: we only include the current source dir, not the include/ subdirectory
# The include/ directory contains target headers for compiled programs, not build headers
target_include_directories(irre-cc PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with math library as required by chibicc
target_link_libraries(irre-cc m)