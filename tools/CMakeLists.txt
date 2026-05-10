# Tools subdirectory: compiler and decompiler
# Built only if ENABLE_TOOLS=ON

add_executable(chip8c
    compiler/src/main.cpp
    compiler/src/lexer.cpp
    compiler/src/ast.cpp
    compiler/src/parser.cpp
)

target_include_directories(chip8c PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/compiler/include
)

add_executable(chip8d
    decompiler/src/main.cpp
)

target_include_directories(chip8d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/decompiler/include
)
