#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <cstdint>

// Represents a single CHIP-8 mnemonic instruction
struct Opcode {
    std::string mnemonic;               // "ADD", "SUB", "AND", etc.
    std::vector<std::string> operands;  // ["V0", "V1"], ["V0", "0x05"], etc.
    
    Opcode() = default;
    Opcode(const std::string& mn, const std::vector<std::string>& ops = {})
        : mnemonic(mn), operands(ops) {}
    
    std::string toString() const;
};

// Maps high-level operations to CHIP-8 mnemonics
class OpCodeMapper {
public:
    OpCodeMapper();

    // Binary operations
    // Returns the mnemonic for the operation, or throws if unsupported
    std::string mapBinaryOp(const std::string& op) const;
    
    // Check if operation is supported
    bool isBinaryOpSupported(const std::string& op) const;
    bool isUnaryOpSupported(const std::string& op) const;
    
    // Categorize operations that require synthesis
    enum class SynthesisCategory {
        DIRECT,           // Direct CHIP-8 opcode
        COMPARISON,       // <, >, ==, !=, <=, >= (skip instruction synthesis)
        LOGICAL,          // &&, || (short-circuit + truth value synthesis)
        UNSUPPORTED,      // *, /, %, etc. (compile error)
    };
    
    SynthesisCategory categorizeOp(const std::string& op) const;
    bool requiresSynthesis(const std::string& op) const;
    
    // Helper methods for categorization
    bool isComparison(const std::string& op) const;
    bool isLogicalOp(const std::string& op) const;
    
    // Built-in functions (flag readers)
    bool isFlagReader(const std::string& func_name) const;
    std::string mapFlagReader(const std::string& func_name) const;

    // Query methods
    int getOperatorPrecedence(const std::string& op) const;

private:
    std::map<std::string, std::string> binary_ops_;  // "+" → "ADD", "-" → "SUB", etc.
    std::set<std::string> comparison_ops_;           // "<", ">", "==", etc.
    std::set<std::string> logical_ops_;              // "&&", "||"
    std::map<std::string, std::string> flag_readers_;  // "get_carry" → "LD VF", etc.
};
