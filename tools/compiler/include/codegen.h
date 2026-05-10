#pragma once

#include "ast.h"
#include "register_allocator.h"
#include "opcode_mapper.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Intermediate representation: a single CHIP-8 instruction
struct Instruction {
    std::string mnemonic;               // "LD", "ADD", "DRW", "CALL", etc.
    std::vector<std::string> operands;  // ["V0", "0x05"], ["V1", "V2"], etc.
    int source_line;                    // For listing file
    std::string comment;                // Optional comment

    Instruction(const std::string& mn, const std::vector<std::string>& ops = {},
                int line = 0, const std::string& cmt = "")
        : mnemonic(mn), operands(ops), source_line(line), comment(cmt) {}

    std::string toString() const;
};

// Symbol table: maps variable/sprite names to their CHIP-8 locations
struct SymbolTable {
    std::map<std::string, uint8_t> variables;   // var_name → Vx register (0-14)
    std::map<std::string, uint16_t> sprites;    // sprite_name → ROM address
    std::map<std::string, uint16_t> functions;  // func_name → ROM address
    std::map<std::string, uint16_t> labels;     // label_name → ROM address
    
    uint8_t next_register = 0;  // V0, V1, ... (max 15, VF is reserved)
    uint16_t next_sprite_addr = 0x200;  // Sprites start after code
    
    bool allocateVariable(const std::string& name);
    bool getVariable(const std::string& name, uint8_t& out_register) const;
    bool addSprite(const std::string& name, uint16_t addr);
    bool getSprite(const std::string& name, uint16_t& out_addr) const;
    
    // Enhanced symbol table: define and resolve with duplicate detection
    bool define(const std::string& type, const std::string& name, uint16_t address);
    bool resolve(const std::string& name, uint16_t& out_address) const;
};

// Unique label generator for control flow
class LabelGenerator {
public:
    std::string generateLabel(const std::string& prefix);  // e.g., "LOOP" → "LOOP_1"
    void reset();
    
private:
    std::map<std::string, int> label_counters_;  // prefix → next ID
};

// Sprite allocator: collects sprite data during AST walk
struct SpriteAllocator {
    struct SpriteData {
        std::string name;
        std::vector<std::string> bytes;  // Store as strings (hex or decimal)
    };
    
    std::vector<SpriteData> sprites;
    
    void addSprite(const std::string& name, const std::vector<std::string>& data);
    bool getSprite(const std::string& name, std::vector<std::string>& out_data) const;
};

// Code generator: AST → Assembly → Binary
class CodeGenerator {
public:
    CodeGenerator();

    // Pass 1: Walk AST and emit intermediate assembly
    bool generateAssembly(const std::shared_ptr<Program>& program,
                          std::vector<Instruction>& out_instructions);

    // Pass 2: Assemble intermediate instructions to binary
    bool assembleToBytes(const std::vector<Instruction>& instructions,
                         std::vector<uint8_t>& out_bytes);

    // Get the symbol table (useful for debugging/listing)
    const SymbolTable& getSymbolTable() const { return symbol_table_; }

    // Error reporting
    const std::string& getLastError() const { return last_error_; }

private:
    SymbolTable symbol_table_;
    LabelGenerator label_gen_;
    SpriteAllocator sprite_alloc_;
    RegisterAllocator reg_allocator_;  // For managing temporaries and user variables
    OpCodeMapper op_mapper_;           // For mapping operations to mnemonics
    std::string last_error_;
    uint16_t current_pc_;  // Current program counter during assembly
    
    // Loop context for break/continue
    struct LoopContext {
        std::string continue_label;
        std::string break_label;
    };
    std::vector<LoopContext> loop_stack_;

    // Helper functions for pass 1 (AST → assembly)
    void emitInstruction(std::vector<Instruction>& instructions,
                        const std::string& mnemonic,
                        const std::vector<std::string>& operands = {},
                        const std::string& comment = "");
    
    // Synthesis helpers for complex operations
    uint8_t synthesizeComparison(const std::string& op, uint8_t left_reg, uint8_t right_reg,
                                std::vector<Instruction>& instructions);
    uint8_t synthesizeUnaryOp(const std::string& op, uint8_t operand_reg,
                             std::vector<Instruction>& instructions);
    
    // Built-in function handling
    bool isBuiltinFunction(const std::string& name) const;
    void generateBuiltinFunctionCall(const std::string& name, 
                                     const std::vector<std::shared_ptr<Expression>>& args,
                                     std::vector<Instruction>& instructions);

    void generateProgramAssembly(const std::shared_ptr<Program>& program,
                                std::vector<Instruction>& instructions);
    void generateDeclarationsAssembly(const std::vector<std::shared_ptr<Declaration>>& decls,
                                     std::vector<Instruction>& instructions);
    void generateFunctionAssembly(const std::shared_ptr<FunctionDef>& func,
                                 std::vector<Instruction>& instructions);
    void generateStatementAssembly(const std::shared_ptr<Statement>& stmt,
                                  std::vector<Instruction>& instructions);
    
    // Enhanced expression generation: returns the register/location holding the result
    uint8_t generateExpressionAssembly(const std::shared_ptr<Expression>& expr,
                                      std::vector<Instruction>& instructions);

    // Helper functions for pass 2 (assembly → binary)
    void collectLabels(const std::vector<Instruction>& instructions);
    bool encodeInstruction(const Instruction& instr, uint16_t& out_opcode) const;
    uint16_t resolveOperand(const std::string& operand) const;
    void setError(const std::string& message);
};
