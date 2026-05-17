# Tools subdirectory: compiler and decompiler
# Built only if ENABLE_TOOLS=ON

add_executable(chip8c
    compiler/src/main.cpp
    compiler/src/lexer.cpp
    compiler/src/ast.cpp
    compiler/src/parser.cpp
    compiler/src/codegen.cpp
    compiler/src/register_allocator.cpp
    compiler/src/opcode_mapper.cpp
)

target_include_directories(chip8c PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(chip8c PRIVATE chip8_debug_map)

# Test executables
add_executable(test_register_allocator
    compiler/test/test_register_allocator.cpp
    compiler/src/register_allocator.cpp
)

target_include_directories(test_register_allocator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(test_opcode_mapper
    compiler/test/test_opcode_mapper.cpp
    compiler/src/opcode_mapper.cpp
)

target_include_directories(test_opcode_mapper PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(test_symbol_table
    compiler/test/test_symbol_table.cpp
    compiler/src/codegen.cpp
    compiler/src/ast.cpp
    compiler/src/register_allocator.cpp
    compiler/src/opcode_mapper.cpp
)

target_include_directories(test_symbol_table PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(test_label_generator
    compiler/test/test_label_generator.cpp
    compiler/src/codegen.cpp
    compiler/src/ast.cpp
    compiler/src/register_allocator.cpp
    compiler/src/opcode_mapper.cpp
)

target_include_directories(test_label_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(chip8d
    decompiler/src/main.cpp
)

target_include_directories(chip8d PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
