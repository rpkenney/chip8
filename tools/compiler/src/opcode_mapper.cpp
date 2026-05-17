#include <chip8/compiler/opcode_mapper.h>
#include <stdexcept>
#include <sstream>

std::string Opcode::toString() const {
    std::ostringstream oss;
    oss << mnemonic;
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i == 0) oss << " ";
        else oss << ", ";
        oss << operands[i];
    }
    return oss.str();
}

OpCodeMapper::OpCodeMapper() {
    // Binary operations (direct mapping)
    binary_ops_["+"] = "ADD";
    binary_ops_["-"] = "SUB";
    binary_ops_["&"] = "AND";
    binary_ops_["|"] = "OR";
    binary_ops_["^"] = "XOR";
    binary_ops_["<<"] = "SHL";
    binary_ops_[">>"] = "SHR";
    
    // Comparison operations (require synthesis in codegen)
    comparison_ops_.insert("==");
    comparison_ops_.insert("!=");
    comparison_ops_.insert("<");
    comparison_ops_.insert(">");
    comparison_ops_.insert("<=");
    comparison_ops_.insert(">=");
    
    // Logical operations (require synthesis: short-circuit + truth value)
    logical_ops_.insert("&&");
    logical_ops_.insert("||");
    
    // Flag readers (built-in functions)
    flag_readers_["get_carry"] = "LD";      // LD Vx, VF (copy carry flag)
    flag_readers_["get_borrow"] = "LD";     // LD Vx, VF (copy borrow flag)
    flag_readers_["get_collision"] = "LD";  // LD Vx, VF (copy collision flag)
}

std::string OpCodeMapper::mapBinaryOp(const std::string& op) const {
    // Reject unsupported operations (multiply, divide, modulo)
    if (op == "*" || op == "/" || op == "%") {
        throw std::runtime_error("Operation not supported in MVP: " + op + 
                                 "\nUse bit shifts (<<, >>) for powers of 2, or refactor your code.");
    }
    
    // Comparisons are synthesized inline during codegen, not mapped here
    if (isComparison(op)) {
        throw std::runtime_error("Comparison '" + op + "' requires synthesis in codegen, not direct mapping");
    }
    
    auto it = binary_ops_.find(op);
    if (it == binary_ops_.end()) {
        throw std::runtime_error("Unknown binary operation: " + op);
    }
    
    return it->second;
}

bool OpCodeMapper::isBinaryOpSupported(const std::string& op) const {
    if (op == "*" || op == "/" || op == "%") {
        return false;
    }
    // Comparisons and logical ops are "supported" but require synthesis
    if (isComparison(op) || isLogicalOp(op)) {
        return true;
    }
    return binary_ops_.count(op) > 0;
}

bool OpCodeMapper::isUnaryOpSupported(const std::string& op) const {
    // Unary NOT requires synthesis
    if (op == "!") {
        return true;
    }
    // Unary minus is direct (NEG instruction or XOR trick)
    if (op == "-") {
        return true;
    }
    return false;
}

bool OpCodeMapper::isComparison(const std::string& op) const {
    return comparison_ops_.count(op) > 0;
}

bool OpCodeMapper::isLogicalOp(const std::string& op) const {
    return logical_ops_.count(op) > 0;
}

OpCodeMapper::SynthesisCategory OpCodeMapper::categorizeOp(const std::string& op) const {
    // Check for unsupported first
    if (op == "*" || op == "/" || op == "%") {
        return SynthesisCategory::UNSUPPORTED;
    }
    
    // Check for comparisons
    if (isComparison(op)) {
        return SynthesisCategory::COMPARISON;
    }
    
    // Check for logical ops
    if (isLogicalOp(op)) {
        return SynthesisCategory::LOGICAL;
    }
    
    // Everything else is direct
    if (binary_ops_.count(op) > 0) {
        return SynthesisCategory::DIRECT;
    }
    
    // Unknown op
    return SynthesisCategory::UNSUPPORTED;
}

bool OpCodeMapper::requiresSynthesis(const std::string& op) const {
    auto cat = categorizeOp(op);
    return cat == SynthesisCategory::COMPARISON || cat == SynthesisCategory::LOGICAL;
}

bool OpCodeMapper::isFlagReader(const std::string& func_name) const {
    return flag_readers_.count(func_name) > 0;
}

std::string OpCodeMapper::mapFlagReader(const std::string& func_name) const {
    auto it = flag_readers_.find(func_name);
    if (it == flag_readers_.end()) {
        throw std::runtime_error("Unknown flag reader: " + func_name);
    }
    return it->second;
}

int OpCodeMapper::getOperatorPrecedence(const std::string& op) const {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return 3;
    if (op == "|") return 4;
    if (op == "^") return 5;
    if (op == "&") return 6;
    if (op == "+" || op == "-") return 7;
    if (op == "<<" || op == ">>") return 8;
    if (op == "*" || op == "/" || op == "%") return 9;
    return 0;  // Unknown operator
}
