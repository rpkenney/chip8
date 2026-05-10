#include "codegen.h"
#include <iostream>
#include <cassert>
#include <sstream>

int main() {
    std::cout << "=== Testing LabelGenerator ===" << std::endl << std::endl;

    // Test 1: Generate sequential labels
    std::cout << "Test 1: Generate sequential labels" << std::endl;
    LabelGenerator gen;
    std::string loop1 = gen.generateLabel("LOOP");
    std::string loop2 = gen.generateLabel("LOOP");
    std::string exit1 = gen.generateLabel("EXIT");
    
    assert(loop1 == "LOOP_1");
    assert(loop2 == "LOOP_2");
    assert(exit1 == "EXIT_1");
    std::cout << "  Generated: " << loop1 << ", " << loop2 << ", " << exit1 << std::endl;
    std::cout << "  ✓ Labels generated uniquely and sequentially" << std::endl << std::endl;

    // Test 2: Different prefixes
    std::cout << "Test 2: Different prefixes" << std::endl;
    std::string else1 = gen.generateLabel("ELSE");
    std::string else2 = gen.generateLabel("ELSE");
    std::string else3 = gen.generateLabel("ELSE");
    
    assert(else1 == "ELSE_1");
    assert(else2 == "ELSE_2");
    assert(else3 == "ELSE_3");
    std::cout << "  Generated: " << else1 << ", " << else2 << ", " << else3 << std::endl;
    std::cout << "  ✓ Different prefixes tracked independently" << std::endl << std::endl;

    // Test 3: Reset
    std::cout << "Test 3: Reset" << std::endl;
    gen.reset();
    std::string loop3 = gen.generateLabel("LOOP");
    assert(loop3 == "LOOP_1");
    std::cout << "  After reset, generated: " << loop3 << std::endl;
    std::cout << "  ✓ Reset clears label counters" << std::endl << std::endl;

    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
