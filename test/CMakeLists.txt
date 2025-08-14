# Test configuration for IRRE

# Configure Catch2 options before adding subdirectory
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "")
set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "")
set(CATCH_CONFIG_ENABLE_BENCHMARKING OFF CACHE BOOL "")

# Add Catch2 from submodule with proper binary dir
add_subdirectory(${CMAKE_SOURCE_DIR}/src/third_party/catch2 ${CMAKE_CURRENT_BINARY_DIR}/catch2 EXCLUDE_FROM_ALL)

# Create test executable
add_executable(irre-test
    test_main.cpp
    test_encoding.cpp
    test_instruction.cpp
)

# Link against irre_core and Catch2
target_link_libraries(irre-test 
    PRIVATE 
    irre_core
    Catch2::Catch2
)

# Include directories
target_include_directories(irre-test 
    PRIVATE 
    ${CMAKE_SOURCE_DIR}/src/irre_core
)

# Enable C++17
target_compile_features(irre-test PRIVATE cxx_std_17)

# Set output directory to build/test/
set_target_properties(irre-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Register tests with CTest
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/src/third_party/catch2/extras)
include(Catch)
catch_discover_tests(irre-test)