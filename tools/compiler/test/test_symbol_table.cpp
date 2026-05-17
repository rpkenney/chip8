#include <chip8/compiler/codegen.h>
#include <cassert>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== Testing SymbolTable ===" << std::endl << std::endl;

    // Test 1: Define and resolve labels
    std::cout << "Test 1: Define and resolve labels" << std::endl;
    SymbolTable table;
    assert(table.define("label", "LOOP_1", 0x200));
    assert(table.define("label", "EXIT_1", 0x210));
    
    uint16_t addr;
    assert(table.resolve("LOOP_1", addr) && addr == 0x200);
    assert(table.resolve("EXIT_1", addr) && addr == 0x210);
    std::cout << "  ✓ Labels defined and resolved correctly" << std::endl << std::endl;

    // Test 2: Duplicate detection (define() throws; no silent false)
    std::cout << "Test 2: Duplicate detection" << std::endl;
    try {
        table.define("label", "LOOP_1", 0x220);
        assert(false && "duplicate label define should throw");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("Duplicate label") != std::string::npos);
    }
    std::cout << "  ✓ Duplicate label rejected" << std::endl << std::endl;

    // Test 3: Define and resolve functions
    std::cout << "Test 3: Define and resolve functions" << std::endl;
    assert(table.define("function", "main", 0x200));
    assert(table.define("function", "draw_helper", 0x230));
    
    assert(table.resolve("main", addr) && addr == 0x200);
    assert(table.resolve("draw_helper", addr) && addr == 0x230);
    std::cout << "  ✓ Functions defined and resolved correctly" << std::endl << std::endl;

    // Test 4: Define and resolve sprites
    std::cout << "Test 4: Define and resolve sprites" << std::endl;
    assert(table.define("sprite", "sprite_ball", 0x300));
    assert(table.define("sprite", "sprite_heart", 0x304));
    
    assert(table.resolve("sprite_ball", addr) && addr == 0x300);
    assert(table.resolve("sprite_heart", addr) && addr == 0x304);
    std::cout << "  ✓ Sprites defined and resolved correctly" << std::endl << std::endl;

    // Test 5: Mixed types
    std::cout << "Test 5: Mixed symbol types" << std::endl;
    SymbolTable mixed;
    assert(mixed.define("label", "LOOP", 0x200));
    assert(mixed.define("function", "LOOP", 0x210));  // Same name, different type (should work)
    assert(mixed.define("sprite", "LOOP", 0x300));
    
    assert(mixed.resolve("LOOP", addr));  // Should resolve (finds label first, then function, then sprite)
    std::cout << "  ✓ Mixed symbol types work correctly" << std::endl << std::endl;

    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
