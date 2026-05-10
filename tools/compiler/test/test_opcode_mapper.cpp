#include "opcode_mapper.h"
#include <iostream>
#include <cassert>

int main() {
    OpCodeMapper mapper;

    std::cout << "=== Testing OpCodeMapper ===" << std::endl << std::endl;

    // Test 1: Supported binary operations
    std::cout << "Test 1: Supported operations" << std::endl;
    assert(mapper.mapBinaryOp("+") == "ADD");
    assert(mapper.mapBinaryOp("-") == "SUB");
    assert(mapper.mapBinaryOp("&") == "AND");
    assert(mapper.mapBinaryOp("|") == "OR");
    assert(mapper.mapBinaryOp("^") == "XOR");
    assert(mapper.mapBinaryOp("<<") == "SHL");
    assert(mapper.mapBinaryOp(">>") == "SHR");
    std::cout << "  ✓ All basic operations map correctly" << std::endl << std::endl;

    // Test 2: Reject unsupported operations
    std::cout << "Test 2: Reject unsupported operations" << std::endl;
    assert(!mapper.isBinaryOpSupported("*"));
    assert(!mapper.isBinaryOpSupported("/"));
    assert(!mapper.isBinaryOpSupported("%"));
    std::cout << "  ✓ Multiply, divide, modulo rejected" << std::endl;
    
    try {
        mapper.mapBinaryOp("*");
        assert(false && "Should have thrown");
    } catch (const std::runtime_error& e) {
        std::cout << "  ✓ Multiply throws: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    // Test 3: Comparisons (require synthesis)
    std::cout << "Test 3: Comparisons (synthesized)" << std::endl;
    assert(mapper.isComparison("<"));
    assert(mapper.isComparison("=="));
    assert(mapper.requiresSynthesis("<="));
    assert(mapper.isBinaryOpSupported(">="));  // supported but synthesized
    std::cout << "  ✓ Comparisons recognized as synthesized" << std::endl << std::endl;

    // Test 4: Logical operations (also synthesized)
    std::cout << "Test 4: Logical operations (synthesized)" << std::endl;
    assert(mapper.isLogicalOp("&&"));
    assert(mapper.isLogicalOp("||"));
    assert(mapper.isBinaryOpSupported("&&"));
    assert(mapper.requiresSynthesis("||"));
    std::cout << "  ✓ Logical operations recognized as synthesized" << std::endl << std::endl;

    // Test 5: Categorization
    std::cout << "Test 5: Operation categorization" << std::endl;
    assert(mapper.categorizeOp("+") == OpCodeMapper::SynthesisCategory::DIRECT);
    assert(mapper.categorizeOp("<") == OpCodeMapper::SynthesisCategory::COMPARISON);
    assert(mapper.categorizeOp("&&") == OpCodeMapper::SynthesisCategory::LOGICAL);
    assert(mapper.categorizeOp("*") == OpCodeMapper::SynthesisCategory::UNSUPPORTED);
    std::cout << "  ✓ Categorization works correctly" << std::endl << std::endl;

    // Test 6: Unary operations
    std::cout << "Test 6: Unary operations" << std::endl;
    assert(mapper.isUnaryOpSupported("!"));
    assert(mapper.isUnaryOpSupported("-"));
    std::cout << "  ✓ Unary operations recognized" << std::endl << std::endl;

    // Test 7: Flag readers
    std::cout << "Test 7: Flag readers" << std::endl;
    assert(mapper.isFlagReader("get_carry"));
    assert(mapper.isFlagReader("get_borrow"));
    assert(mapper.isFlagReader("get_collision"));
    assert(!mapper.isFlagReader("unknown_func"));
    assert(mapper.mapFlagReader("get_carry") == "LD");
    std::cout << "  ✓ Flag readers recognized" << std::endl << std::endl;

    // Test 8: Operator precedence
    std::cout << "Test 8: Operator precedence" << std::endl;
    int prec_add = mapper.getOperatorPrecedence("+");
    int prec_mul = mapper.getOperatorPrecedence("*");
    assert(prec_mul > prec_add);
    std::cout << "  + precedence: " << prec_add << std::endl;
    std::cout << "  * precedence: " << prec_mul << std::endl;
    std::cout << "  ✓ Precedence correct (* > +)" << std::endl << std::endl;

    // Test 9: Opcode struct
    std::cout << "Test 9: Opcode struct" << std::endl;
    Opcode add_op("ADD", {"V0", "V1"});
    std::cout << "  Opcode: " << add_op.toString() << std::endl;
    assert(add_op.toString() == "ADD V0, V1");
    std::cout << "  ✓ Opcode toString works" << std::endl << std::endl;

    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
