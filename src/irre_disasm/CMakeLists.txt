add_executable(irre-disasm main.cpp)
target_link_libraries(irre-disasm irre_core)
target_include_directories(irre-disasm PRIVATE ${CMAKE_SOURCE_DIR}/src)