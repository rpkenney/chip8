add_library(chip8_debug_map STATIC
    debug_map.cpp
)

target_include_directories(chip8_debug_map PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(chip8_debug_map PUBLIC cxx_std_17)

add_executable(test_debug_map
    test/test_debug_map.cpp
)
target_link_libraries(test_debug_map PRIVATE chip8_debug_map)
target_compile_features(test_debug_map PRIVATE cxx_std_17)
