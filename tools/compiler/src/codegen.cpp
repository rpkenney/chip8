#include "codegen.h"
#include <sstream>

// Instruction::toString() - pretty-print an instruction
std::string Instruction::toString() const {
    std::ostringstream oss;
    oss << mnemonic;
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i == 0) oss << " ";
        else oss << ", ";
        oss << operands[i];
    }
    if (!comment.empty()) {
        oss << " ; " << comment;
    }
    return oss.str();
}

// SymbolTable methods
bool SymbolTable::allocateVariable(const std::string& name) {
    if (next_register >= 15) {  // V0-VE only (VF is reserved)
        return false;
    }
    variables[name] = next_register++;
    return true;
}

bool SymbolTable::getVariable(const std::string& name, uint8_t& out_register) const {
    auto it = variables.find(name);
    if (it != variables.end()) {
        out_register = it->second;
        return true;
    }
    return false;
}

bool SymbolTable::addSprite(const std::string& name, uint16_t addr) {
    sprites[name] = addr;
    return true;
}

bool SymbolTable::getSprite(const std::string& name, uint16_t& out_addr) const {
    auto it = sprites.find(name);
    if (it != sprites.end()) {
        out_addr = it->second;
        return true;
    }
    return false;
}

// CodeGenerator methods
CodeGenerator::CodeGenerator() : current_pc_(0x200) {}

bool CodeGenerator::generateAssembly(const std::shared_ptr<Program>& program,
                                     std::vector<Instruction>& out_instructions) {
    if (!program) {
        setError("Program is null");
        return false;
    }

    try {
        generateProgramAssembly(program, out_instructions);
        return true;
    } catch (const std::exception& e) {
        setError(e.what());
        return false;
    }
}

bool CodeGenerator::assembleToBytes(const std::vector<Instruction>& instructions,
                                    std::vector<uint8_t>& out_bytes) {
    try {
        current_pc_ = 0x200;
        out_bytes.clear();

        // Emit code starting at 0x200
        for (const auto& instr : instructions) {
            uint16_t opcode = 0;
            if (!encodeInstruction(instr, opcode)) {
                setError("Failed to encode instruction: " + instr.toString());
                return false;
            }

            // Convert opcode to big-endian bytes
            out_bytes.push_back((opcode >> 8) & 0xFF);
            out_bytes.push_back(opcode & 0xFF);
            current_pc_ += 2;
        }

        return true;
    } catch (const std::exception& e) {
        setError(e.what());
        return false;
    }
}

void CodeGenerator::emitInstruction(std::vector<Instruction>& instructions,
                                   const std::string& mnemonic,
                                   const std::vector<std::string>& operands,
                                   const std::string& comment) {
    instructions.push_back(Instruction(mnemonic, operands, 0, comment));
}

void CodeGenerator::generateProgramAssembly(const std::shared_ptr<Program>& program,
                                           std::vector<Instruction>& instructions) {
    // Step 1: Allocate variables
    for (const auto& decl : program->declarations) {
        if (auto var_decl = std::dynamic_pointer_cast<VariableDecl>(decl)) {
            for (const auto& name : var_decl->names) {
                if (!symbol_table_.allocateVariable(name)) {
                    throw std::runtime_error("Too many variables (max 15)");
                }
            }
        }
    }

    // Step 2: Emit function code
    for (const auto& func : program->functions) {
        generateFunctionAssembly(func, instructions);
    }

    // Step 3: Emit sprite data at end
    // (TBD: will handle in next iteration)

    emitInstruction(instructions, "END", {}, "Program end");
}

void CodeGenerator::generateDeclarationsAssembly(const std::vector<std::shared_ptr<Declaration>>& decls,
                                                std::vector<Instruction>& instructions) {
    // TBD: Handle sprite declarations and emit data
    for (const auto& decl : decls) {
        if (auto sprite_decl = std::dynamic_pointer_cast<SpriteDecl>(decl)) {
            // Store sprite data for later
            symbol_table_.addSprite(sprite_decl->name, 0x300);  // Placeholder
        }
    }
}

void CodeGenerator::generateFunctionAssembly(const std::shared_ptr<FunctionDef>& func,
                                            std::vector<Instruction>& instructions) {
    if (!func) return;

    emitInstruction(instructions, "LABEL", {func->name}, "Function: " + func->name);
    
    for (const auto& stmt : func->body) {
        generateStatementAssembly(stmt, instructions);
    }

    // TBD: Add return instruction if needed
}

void CodeGenerator::generateStatementAssembly(const std::shared_ptr<Statement>& stmt,
                                             std::vector<Instruction>& instructions) {
    if (!stmt) return;

    if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        generateExpressionAssembly(expr_stmt->expr, instructions);
    } else if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
        generateExpressionAssembly(assign->value, instructions);
        emitInstruction(instructions, "ASSIGN", {assign->name}, "Assignment: " + assign->name);
    } else if (auto if_stmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        generateExpressionAssembly(if_stmt->condition, instructions);
        emitInstruction(instructions, "JNE", {"LABEL_ELSE"}, "If condition");
        for (const auto& s : if_stmt->thenBranch) {
            generateStatementAssembly(s, instructions);
        }
        if (!if_stmt->elseBranch.empty()) {
            emitInstruction(instructions, "LABEL_ELSE", {}, "Else branch");
            for (const auto& s : if_stmt->elseBranch) {
                generateStatementAssembly(s, instructions);
            }
        }
    } else if (auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        emitInstruction(instructions, "LABEL_LOOP", {}, "While condition");
        generateExpressionAssembly(while_stmt->condition, instructions);
        for (const auto& s : while_stmt->body) {
            generateStatementAssembly(s, instructions);
        }
        emitInstruction(instructions, "JUMP", {"LABEL_LOOP"}, "Loop back");
    }
    // TBD: Handle break, continue, return
}

void CodeGenerator::generateExpressionAssembly(const std::shared_ptr<Expression>& expr,
                                              std::vector<Instruction>& instructions) {
    if (!expr) return;

    if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
        emitInstruction(instructions, "LITERAL", {lit->value}, "Load literal");
    } else if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
        emitInstruction(instructions, "LOAD_VAR", {ident->name}, "Load variable");
    } else if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        generateExpressionAssembly(binop->left, instructions);
        generateExpressionAssembly(binop->right, instructions);
        emitInstruction(instructions, "BINOP", {binop->op}, "Binary operation: " + binop->op);
    } else if (auto call = std::dynamic_pointer_cast<FunctionCall>(expr)) {
        for (const auto& arg : call->args) {
            generateExpressionAssembly(arg, instructions);
        }
        emitInstruction(instructions, "CALL", {call->name}, "Function call: " + call->name);
    }
    // TBD: Handle unary ops
}

bool CodeGenerator::encodeInstruction(const Instruction& instr, uint16_t& out_opcode) const {
    // TBD: Implement actual CHIP-8 opcode encoding
    // For now, just encode a placeholder
    if (instr.mnemonic == "END") {
        out_opcode = 0x0000;  // Placeholder
        return true;
    }
    // More encodings to come...
    return true;
}

void CodeGenerator::setError(const std::string& message) {
    last_error_ = message;
}
