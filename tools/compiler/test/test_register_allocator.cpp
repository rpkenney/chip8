#include <chip8/compiler/register_allocator.h>
#include <iostream>
#include <cassert>

int main() {
    RegisterAllocator allocator;

    std::cout << "=== Testing RegisterAllocator ===" << std::endl << std::endl;

    // Test 1: Allocate user variables
    std::cout << "Test 1: Allocate 5 user variables" << std::endl;
    uint8_t x_reg = allocator.allocateUserVariable("x");
    uint8_t y_reg = allocator.allocateUserVariable("y");
    uint8_t z_reg = allocator.allocateUserVariable("z");
    uint8_t a_reg = allocator.allocateUserVariable("a");
    uint8_t b_reg = allocator.allocateUserVariable("b");

    std::cout << "  x → V" << (int)x_reg << std::endl;
    std::cout << "  y → V" << (int)y_reg << std::endl;
    std::cout << "  z → V" << (int)z_reg << std::endl;
    std::cout << "  a → V" << (int)a_reg << std::endl;
    std::cout << "  b → V" << (int)b_reg << std::endl;
    assert(x_reg == 0 && y_reg == 1 && z_reg == 2 && a_reg == 3 && b_reg == 4);
    std::cout << "  ✓ Greedy allocation works" << std::endl << std::endl;

    // Test 2: Query user variables
    std::cout << "Test 2: Query user variables" << std::endl;
    assert(allocator.isUserVariable("x") == true);
    assert(allocator.isUserVariable("unknown") == false);
    assert(allocator.getUserVariableRegister("x") == 0);
    std::cout << "  ✓ User variable queries work" << std::endl << std::endl;

    // Test 3: Allocate temporaries
    std::cout << "Test 3: Allocate temporaries" << std::endl;
    std::cout << "  Available temps before: " << allocator.getNumAvailableTemps() << std::endl;
    uint8_t temp1 = allocator.allocateTemporary();
    uint8_t temp2 = allocator.allocateTemporary();
    std::cout << "  temp1 → V" << (int)temp1 << std::endl;
    std::cout << "  temp2 → V" << (int)temp2 << std::endl;
    std::cout << "  Available temps after: " << allocator.getNumAvailableTemps() << std::endl;
    assert(temp1 >= 5 && temp2 >= 5 && temp1 != temp2);
    std::cout << "  ✓ Temporary allocation works" << std::endl << std::endl;

    // Test 4: Free temporaries
    std::cout << "Test 4: Free temporaries" << std::endl;
    int temps_before = allocator.getNumAvailableTemps();
    allocator.freeTemporary(temp1);
    int temps_after = allocator.getNumAvailableTemps();
    std::cout << "  Available temps before free: " << temps_before << std::endl;
    std::cout << "  Available temps after free: " << temps_after << std::endl;
    assert(temps_after == temps_before + 1);
    std::cout << "  ✓ Free temporary works" << std::endl << std::endl;

    // Test 5: Spill allocation
    std::cout << "Test 5: Spill allocation" << std::endl;
    uint16_t spill1 = allocator.allocateSpill();
    uint16_t spill2 = allocator.allocateSpill();
    std::cout << "  spill1 → 0x" << std::hex << spill1 << std::dec << std::endl;
    std::cout << "  spill2 → 0x" << std::hex << spill2 << std::dec << std::endl;
    assert(spill1 == 0xD00 && spill2 == 0xD01);
    std::cout << "  ✓ Spill allocation works" << std::endl << std::endl;

    // Test 6: Spill user variables when out of registers
    std::cout << "Test 6: Spill user variables" << std::endl;
    RegisterAllocator allocator3;
    for (int i = 0; i < 15; ++i) {
        allocator3.allocateUserVariable("reg_var_" + std::to_string(i));
    }
    std::cout << "  Allocated 15 variables in registers" << std::endl;
    
    // Now allocate one more; it should spill
    allocator3.allocateUserVariable("spilled_var");
    assert(allocator3.isUserVariable("spilled_var") == true);
    assert(allocator3.isUserVariableSpilled("spilled_var") == true);
    std::cout << "  ✓ User variable spilled to memory at 0x" 
              << std::hex << allocator3.getUserVariableSpillAddress("spilled_var") 
              << std::dec << std::endl;
    std::cout << std::endl;

    // Test 7: Error on out of spill memory
    std::cout << "Test 7: Error on spill overflow" << std::endl;
    RegisterAllocator allocator4;
    for (int i = 0; i < 15 + (0xE00 - 0xD00); ++i) {
        try {
            allocator4.allocateUserVariable("huge_var_" + std::to_string(i));
        } catch (const std::runtime_error& e) {
            std::cout << "  ✓ Correctly threw error: " << e.what() << std::endl;
            break;
        }
    }
    std::cout << std::endl;

    // Test 8: Reset
    std::cout << "Test 8: Reset allocator" << std::endl;
    allocator.reset();
    std::cout << "  User variables after reset: " << allocator.getNumUserVariables() << std::endl;
    assert(allocator.getNumUserVariables() == 0);
    std::cout << "  ✓ Reset works" << std::endl << std::endl;

    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
