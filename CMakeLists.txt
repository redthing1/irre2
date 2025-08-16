cmake_minimum_required(VERSION 3.20)
project(irre LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# set output directories to build-dir/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# find clang-tidy and enable if available
find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE};
        --warnings-as-errors=*;
        --header-filter=.*
    )
    set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY_EXE};
        --warnings-as-errors=*;
        --header-filter=.*
    )
else()
    message(STATUS "clang-tidy not found")
endif()

# find clang-format and create format/check-format targets if available
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if(CLANG_FORMAT_EXE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXE}")
    # collect all source files, excluding third_party
    file(GLOB_RECURSE ALL_SOURCE_FILES 
         ${CMAKE_SOURCE_DIR}/src/irre_*/*.cpp
         ${CMAKE_SOURCE_DIR}/src/irre_*/*.hpp
         ${CMAKE_SOURCE_DIR}/src/irre_*/*.h
         ${CMAKE_SOURCE_DIR}/src/irre_*/*.c
         ${CMAKE_SOURCE_DIR}/test/*.cpp
         ${CMAKE_SOURCE_DIR}/test/*.hpp
         ${CMAKE_SOURCE_DIR}/test/*.h)
    
    # filter out third_party files
    list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX ".*third_party.*")
    
    # target to format code in-place
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
    )
    
    # target to check formatting without changing files (fails build if not formatted)
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code formatting with clang-format"
    )
else()
    message(STATUS "clang-format not found")
endif()

add_subdirectory(src/irre_core)
add_subdirectory(src/irre_asm)
add_subdirectory(src/irre_disasm)
add_subdirectory(src/irre_emu)
add_subdirectory(src/irre_cc)

# Testing
option(BUILD_TESTING "Enable testing" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()