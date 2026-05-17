#include <chip8/compiler/codegen.h>
#include <sstream>
#include <stdexcept>
#include <set>
#include <iomanip>

// Forward declarations for helper functions
static uint16_t parseLiteral(const std::string& lit);

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

bool SymbolTable::define(const std::string& type, const std::string& name, uint16_t address) {
    if (type == "label") {
        if (labels.count(name)) {
            throw std::runtime_error("Duplicate label: " + name);
        }
        labels[name] = address;
        return true;
    } else if (type == "function") {
        if (functions.count(name)) {
            throw std::runtime_error("Duplicate function: " + name);
        }
        functions[name] = address;
        return true;
    } else if (type == "sprite") {
        if (sprites.count(name)) {
            throw std::runtime_error("Duplicate sprite: " + name);
        }
        sprites[name] = address;
        return true;
    }
    return false;
}

bool SymbolTable::resolve(const std::string& name, uint16_t& out_address) const {
    // Try labels first
    auto it_label = labels.find(name);
    if (it_label != labels.end()) {
        out_address = it_label->second;
        return true;
    }
    
    // Try functions
    auto it_func = functions.find(name);
    if (it_func != functions.end()) {
        out_address = it_func->second;
        return true;
    }
    
    // Try sprites
    auto it_sprite = sprites.find(name);
    if (it_sprite != sprites.end()) {
        out_address = it_sprite->second;
        return true;
    }
    
    return false;
}

// LabelGenerator methods
std::string LabelGenerator::generateLabel(const std::string& prefix) {
    int& counter = label_counters_[prefix];
    counter++;
    return prefix + "_" + std::to_string(counter);
}

void LabelGenerator::reset() {
    label_counters_.clear();
}

// SpriteAllocator methods
void SpriteAllocator::addSprite(const std::string& name, const std::vector<std::string>& data) {
    sprites.push_back({name, data});
}

bool SpriteAllocator::getSprite(const std::string& name, std::vector<std::string>& out_data) const {
    for (const auto& sprite : sprites) {
        if (sprite.name == name) {
            out_data = sprite.bytes;
            return true;
        }
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

        // === PASS 2.1: First Scan - Collect Labels & Allocate Sprites ===
        collectLabels(instructions);

        // === PASS 2.4: Second Scan - Symbol Resolution & Emission (Code) ===
        for (const auto& instr : instructions) {
            // Skip pseudo-instructions (LABEL, END)
            if (instr.mnemonic == "LABEL" || instr.mnemonic == "END") {
                continue;
            }

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

        // === PASS 2.4 (continued): Emit Sprite Data ===
        for (const auto& sprite : sprite_alloc_.sprites) {
            // Each sprite byte is stored as a hex string in the SpriteAllocator
            for (const auto& byte_str : sprite.bytes) {
                uint8_t byte_val = parseLiteral(byte_str);
                out_bytes.push_back(byte_val);
            }
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
    instructions.push_back(Instruction(mnemonic, operands, current_source_line_, comment));
}

uint8_t CodeGenerator::synthesizeComparison(const std::string& op, uint8_t left_reg, uint8_t right_reg,
                                           std::vector<Instruction>& instructions) {
    // Allocate result register (will hold 0 or 1)
    uint8_t result_reg = reg_allocator_.allocateTemporary();
    if (result_reg == 0xFF) {
        throw std::runtime_error("Out of spill memory for comparison result");
    }
    
    // Allocate temp registers for comparison (don't modify operands!)
    uint8_t temp_left = reg_allocator_.allocateTemporary();
    if (temp_left == 0xFF) {
        throw std::runtime_error("Out of spill memory for comparison temporaries");
    }
    
    std::string left_name = "V" + std::to_string(left_reg);
    std::string right_name = "V" + std::to_string(right_reg);
    std::string result_name = "V" + std::to_string(result_reg);
    std::string temp_left_name = "V" + std::to_string(temp_left);
    
    // Initialize result to 0 (false)
    emitInstruction(instructions, "LD", {result_name, "0"}, "Comparison: initialize to 0");
    
    // Copy left operand to temp (preserve original)
    emitInstruction(instructions, "LD", {temp_left_name, left_name}, "Copy left operand for comparison");
    
    if (op == "==") {
        // x == y: XOR gives 0 if equal
        emitInstruction(instructions, "XOR", {temp_left_name, right_name}, "Compare ==: XOR");
        emitInstruction(instructions, "SNE", {temp_left_name, "0"}, "Skip if not equal");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if equal");
        
    } else if (op == "!=") {
        // x != y: Opposite of ==
        emitInstruction(instructions, "XOR", {temp_left_name, right_name}, "Compare !=: XOR");
        emitInstruction(instructions, "SE", {temp_left_name, "0"}, "Skip if equal");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if not equal");
        
    } else if (op == "<") {
        // x < y: SUB Vx, Vy sets VF = 1 if borrow (underflow), i.e., Vx < Vy
        emitInstruction(instructions, "SUB", {temp_left_name, right_name}, "Compare <: SUB");
        emitInstruction(instructions, "SNE", {"VF", "0"}, "Skip if VF is 0 (no borrow)");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if x < y");
        
    } else if (op == ">") {
        // x > y: SUB Vy, Vx sets VF = 1 if Vy < Vx, i.e., x > y
        // Need another temp for right
        uint8_t temp_right = reg_allocator_.allocateTemporary();
        if (temp_right == 0xFF) {
            throw std::runtime_error("Out of spill memory for comparison temporaries");
        }
        std::string temp_right_name = "V" + std::to_string(temp_right);
        
        emitInstruction(instructions, "LD", {temp_right_name, right_name}, "Copy right operand");
        emitInstruction(instructions, "SUB", {temp_right_name, temp_left_name}, "Compare >: SUB");
        emitInstruction(instructions, "SNE", {"VF", "0"}, "Skip if VF is 0 (no borrow)");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if x > y");
        reg_allocator_.freeTemporary(temp_right);
        
    } else if (op == "<=") {
        // x <= y: NOT (x > y)
        uint8_t temp_right = reg_allocator_.allocateTemporary();
        if (temp_right == 0xFF) {
            throw std::runtime_error("Out of spill memory for comparison temporaries");
        }
        std::string temp_right_name = "V" + std::to_string(temp_right);
        
        emitInstruction(instructions, "LD", {temp_right_name, right_name}, "Copy right operand");
        emitInstruction(instructions, "SUB", {temp_right_name, temp_left_name}, "Compare <=: SUB");
        emitInstruction(instructions, "SE", {"VF", "0"}, "Skip if VF is 0 (x <= y)");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if x <= y");
        reg_allocator_.freeTemporary(temp_right);
        
    } else if (op == ">=") {
        // x >= y: NOT (x < y)
        emitInstruction(instructions, "SUB", {temp_left_name, right_name}, "Compare >=: SUB");
        emitInstruction(instructions, "SE", {"VF", "0"}, "Skip if VF is 0 (x >= y)");
        emitInstruction(instructions, "LD", {result_name, "1"}, "Set result to 1 if x >= y");
    } else {
        throw std::runtime_error("Unknown comparison operator: " + op);
    }
    
    // Free temp registers
    reg_allocator_.freeTemporary(temp_left);
    
    return result_reg;
}

uint8_t CodeGenerator::synthesizeUnaryOp(const std::string& op, uint8_t operand_reg,
                                        std::vector<Instruction>& instructions) {
    uint8_t result_reg = operand_reg;  // Can modify in-place or allocate new
    std::string operand_name = "V" + std::to_string(operand_reg);
    
    if (op == "!") {
        // Logical NOT: convert 0 → 1, non-zero → 0
        // Initialize result to 1 (assume true)
        uint8_t result = reg_allocator_.allocateTemporary();
        if (result == 0xFF) {
            throw std::runtime_error("Out of spill memory for unary NOT");
        }
        
        std::string result_name = "V" + std::to_string(result);
        emitInstruction(instructions, "LD", {result_name, "1"}, "Unary !: initialize to 1");
        emitInstruction(instructions, "SNE", {operand_name, "0"}, "Skip if operand is 0");
        emitInstruction(instructions, "LD", {result_name, "0"}, "Set to 0 if operand is non-zero");
        
        return result;
        
    } else if (op == "-") {
        // Unary negation: two's complement (NOT + 1)
        // Allocate temp, don't modify operand!
        uint8_t result = reg_allocator_.allocateTemporary();
        if (result == 0xFF) {
            throw std::runtime_error("Out of spill memory for unary negation");
        }
        
        std::string result_name = "V" + std::to_string(result);
        
        // Copy operand to temp
        emitInstruction(instructions, "LD", {result_name, operand_name}, "Unary -: copy operand");
        // XOR with 0xFF to flip bits, then ADD 1
        emitInstruction(instructions, "XOR", {result_name, "0xFF"}, "Unary -: XOR with 0xFF");
        emitInstruction(instructions, "ADD", {result_name, "1"}, "Unary -: add 1 (two's complement)");
        
        return result;
    } else {
        throw std::runtime_error("Unknown unary operator: " + op);
    }
}

bool CodeGenerator::isBuiltinFunction(const std::string& name) const {
    static const std::set<std::string> builtins = {
        "clear_screen",
        "draw_sprite",
        "set_delay",
        "get_delay",
        "set_sound",
        "wait_key",
        "get_key"
    };
    return builtins.count(name) > 0;
}

void CodeGenerator::generateBuiltinFunctionCall(const std::string& name, 
                                               const std::vector<std::shared_ptr<Expression>>& args,
                                               std::vector<Instruction>& instructions) {
    if (name == "clear_screen") {
        // CLS (no args)
        emitInstruction(instructions, "CLS", {}, "Clear screen");
        
    } else if (name == "draw_sprite") {
        // draw_sprite(x, y, sprite_name)
        if (args.size() != 3) {
            throw std::runtime_error("draw_sprite requires 3 arguments");
        }
        
        // Evaluate x into V0
        uint8_t x_reg = generateExpressionAssembly(args[0], instructions);
        if (x_reg != 0) {  // Move to V0 if needed
            emitInstruction(instructions, "LD", {"V0", "V" + std::to_string(x_reg)}, "Set x for draw_sprite");
            if (reg_allocator_.isTemporaryInRegister(x_reg)) {
                reg_allocator_.freeTemporary(x_reg);
            }
        }
        
        // Evaluate y into V1
        uint8_t y_reg = generateExpressionAssembly(args[1], instructions);
        if (y_reg != 1) {  // Move to V1 if needed
            emitInstruction(instructions, "LD", {"V1", "V" + std::to_string(y_reg)}, "Set y for draw_sprite");
            if (reg_allocator_.isTemporaryInRegister(y_reg)) {
                reg_allocator_.freeTemporary(y_reg);
            }
        }
        
        // Evaluate sprite (could be identifier or expression) and load into I
        // For now, just get the name and emit a placeholder
        if (auto sprite_ident = std::dynamic_pointer_cast<Identifier>(args[2])) {
            emitInstruction(instructions, "LD", {"I", sprite_ident->name}, "Load sprite address into I");
        } else {
            uint8_t sprite_reg = generateExpressionAssembly(args[2], instructions);
            emitInstruction(instructions, "LD", {"I", "V" + std::to_string(sprite_reg)}, "Load sprite address into I");
            if (reg_allocator_.isTemporaryInRegister(sprite_reg)) {
                reg_allocator_.freeTemporary(sprite_reg);
            }
        }
        
        // DRW V0, V1, N (height is hardcoded to 4 for MVP)
        emitInstruction(instructions, "DRW", {"V0", "V1", "4"}, "Draw sprite");
        
    } else if (name == "set_delay") {
        // set_delay(value)
        if (args.size() != 1) {
            throw std::runtime_error("set_delay requires 1 argument");
        }
        
        uint8_t val_reg = generateExpressionAssembly(args[0], instructions);
        if (val_reg != 0) {
            emitInstruction(instructions, "LD", {"V0", "V" + std::to_string(val_reg)}, "Set value for set_delay");
            if (reg_allocator_.isTemporaryInRegister(val_reg)) {
                reg_allocator_.freeTemporary(val_reg);
            }
        }
        
        emitInstruction(instructions, "LD", {"DT", "V0"}, "Set delay timer");
        
    } else if (name == "get_delay") {
        // get_delay() - returns current delay timer in V0
        emitInstruction(instructions, "LD", {"V0", "DT"}, "Get delay timer");
        
    } else if (name == "set_sound") {
        // set_sound(value)
        if (args.size() != 1) {
            throw std::runtime_error("set_sound requires 1 argument");
        }
        
        uint8_t val_reg = generateExpressionAssembly(args[0], instructions);
        if (val_reg != 0) {
            emitInstruction(instructions, "LD", {"V0", "V" + std::to_string(val_reg)}, "Set value for set_sound");
            if (reg_allocator_.isTemporaryInRegister(val_reg)) {
                reg_allocator_.freeTemporary(val_reg);
            }
        }
        
        emitInstruction(instructions, "LD", {"ST", "V0"}, "Set sound timer");
        
    } else if (name == "wait_key") {
        // wait_key() - blocks until key pressed
        emitInstruction(instructions, "LD", {"V0", "K"}, "Wait for key");
        
    } else if (name == "get_key") {
        // get_key() - returns last key pressed (non-blocking)
        throw std::runtime_error("get_key not yet implemented");
        
    } else {
        throw std::runtime_error("Unknown built-in function: " + name);
    }
}

void CodeGenerator::generateProgramAssembly(const std::shared_ptr<Program>& program,
                                           std::vector<Instruction>& instructions) {
    // Step 1: Allocate user variables using register allocator
    for (const auto& decl : program->declarations) {
        if (auto var_decl = std::dynamic_pointer_cast<VariableDecl>(decl)) {
            for (const auto& name : var_decl->names) {
                reg_allocator_.allocateUserVariable(name);
                // Track in symbol table for debugging (optional)
            }
        }
    }

    // Step 2: Collect sprite data
    generateDeclarationsAssembly(program->declarations, instructions);

    // Step 3: Emit function code
    for (const auto& func : program->functions) {
        generateFunctionAssembly(func, instructions);
    }

    emitInstruction(instructions, "END", {}, "Program end");
}

void CodeGenerator::generateDeclarationsAssembly(const std::vector<std::shared_ptr<Declaration>>& decls,
                                                std::vector<Instruction>& instructions) {
    // Process sprite declarations and collect sprite data
    for (const auto& decl : decls) {
        if (auto sprite_decl = std::dynamic_pointer_cast<SpriteDecl>(decl)) {
            // Store sprite data in sprite allocator (address allocation deferred to Pass 2)
            sprite_alloc_.addSprite(sprite_decl->name, sprite_decl->bytes);
        }
    }
}

void CodeGenerator::generateFunctionAssembly(const std::shared_ptr<FunctionDef>& func,
                                            std::vector<Instruction>& instructions) {
    if (!func) return;

    // Function address will be resolved in Pass 2.1 when we collect labels
    emitInstruction(instructions, "LABEL", {func->name}, "Function: " + func->name);
    
    for (const auto& stmt : func->body) {
        generateStatementAssembly(stmt, instructions);
    }

    // TBD: Add return instruction if needed
}

void CodeGenerator::generateStatementAssembly(const std::shared_ptr<Statement>& stmt,
                                             std::vector<Instruction>& instructions) {
    if (!stmt) return;

    const int saved_line = current_source_line_;
    current_source_line_ = stmt->line;

    if (auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        uint8_t result = generateExpressionAssembly(expr_stmt->expr, instructions);
        if (result != 0xFF && reg_allocator_.isTemporaryInRegister(result)) {
            reg_allocator_.freeTemporary(result);  // Free the result if it's a temp
        }
    } else if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
        uint8_t value_reg = generateExpressionAssembly(assign->value, instructions);
        if (value_reg == 0xFF) {
            throw std::runtime_error("Failed to evaluate assignment RHS");
        }
        
        // Assign result to variable
        if (!reg_allocator_.isUserVariable(assign->name)) {
            throw std::runtime_error("Variable not declared: " + assign->name);
        }
        
        if (reg_allocator_.isInRegister(assign->name)) {
            uint8_t var_reg = reg_allocator_.getRegister(assign->name);
            std::string var_name = "V" + std::to_string(var_reg);
            std::string val_name = "V" + std::to_string(value_reg);
            
            // Move result to variable's register
            emitInstruction(instructions, "LD", {var_name, val_name}, 
                          "Assignment: " + assign->name);
        } else if (reg_allocator_.isSpilled(assign->name)) {
            // Store to spill memory
            uint16_t spill_addr = reg_allocator_.getSpillAddress(assign->name);
            std::string val_name = "V" + std::to_string(value_reg);
            
            emitInstruction(instructions, "ST", {val_name, "0x" + std::to_string(spill_addr)}, 
                          "Store to spill: " + assign->name);
        }
        
        if (reg_allocator_.isTemporaryInRegister(value_reg)) {
            reg_allocator_.freeTemporary(value_reg);
        }
        
    } else if (auto if_stmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        std::string else_label = label_gen_.generateLabel("ELSE");
        std::string exit_label = label_gen_.generateLabel("EXIT");
        
        uint8_t cond_reg = generateExpressionAssembly(if_stmt->condition, instructions);
        if (cond_reg == 0xFF) {
            throw std::runtime_error("Failed to evaluate if condition");
        }
        
        std::string cond_name = "V" + std::to_string(cond_reg);
        emitInstruction(instructions, "SNE", {cond_name, "0"}, "If condition (check non-zero)");
        emitInstruction(instructions, "JUMP", {else_label}, "If condition false, go to else");
        
        if (reg_allocator_.isTemporaryInRegister(cond_reg)) {
            reg_allocator_.freeTemporary(cond_reg);
        }
        
        for (const auto& s : if_stmt->thenBranch) {
            generateStatementAssembly(s, instructions);
        }
        
        if (!if_stmt->elseBranch.empty()) {
            emitInstruction(instructions, "JUMP", {exit_label}, "Jump to exit");
            emitInstruction(instructions, "LABEL", {else_label}, "Else branch");
            for (const auto& s : if_stmt->elseBranch) {
                generateStatementAssembly(s, instructions);
            }
            emitInstruction(instructions, "LABEL", {exit_label}, "Exit if");
        } else {
            emitInstruction(instructions, "LABEL", {else_label}, "Exit if (no else)");
        }
    } else if (auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        std::string loop_label = label_gen_.generateLabel("LOOP");
        std::string exit_label = label_gen_.generateLabel("EXIT");
        
        // Push loop context for break/continue
        loop_stack_.push_back({loop_label, exit_label});
        
        emitInstruction(instructions, "LABEL", {loop_label}, "While loop");
        
        uint8_t cond_reg = generateExpressionAssembly(while_stmt->condition, instructions);
        if (cond_reg == 0xFF) {
            throw std::runtime_error("Failed to evaluate while condition");
        }
        
        std::string cond_name = "V" + std::to_string(cond_reg);
        emitInstruction(instructions, "SNE", {cond_name, "0"}, "While condition (check non-zero)");
        emitInstruction(instructions, "JUMP", {exit_label}, "Exit loop if false");
        
        if (reg_allocator_.isTemporaryInRegister(cond_reg)) {
            reg_allocator_.freeTemporary(cond_reg);
        }
        
        for (const auto& s : while_stmt->body) {
            generateStatementAssembly(s, instructions);
        }
        
        emitInstruction(instructions, "JUMP", {loop_label}, "Loop back");
        emitInstruction(instructions, "LABEL", {exit_label}, "Exit while loop");
        
        // Pop loop context
        loop_stack_.pop_back();
        
    } else if (auto break_stmt = std::dynamic_pointer_cast<BreakStatement>(stmt)) {
        if (loop_stack_.empty()) {
            throw std::runtime_error("break statement outside of loop");
        }
        emitInstruction(instructions, "JUMP", {loop_stack_.back().break_label}, "Break from loop");
        
    } else if (auto continue_stmt = std::dynamic_pointer_cast<ContinueStatement>(stmt)) {
        if (loop_stack_.empty()) {
            throw std::runtime_error("continue statement outside of loop");
        }
        emitInstruction(instructions, "JUMP", {loop_stack_.back().continue_label}, "Continue loop");
        
    } else if (auto return_stmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        // Emit RET instruction
        emitInstruction(instructions, "RET", {}, "Return from function");
    }

    current_source_line_ = saved_line;
}

uint8_t CodeGenerator::generateExpressionAssembly(const std::shared_ptr<Expression>& expr,
                                                 std::vector<Instruction>& instructions) {
    if (!expr) return 0xFF;  // Invalid register

    if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
        // Allocate a temporary for the literal
        uint8_t temp = reg_allocator_.allocateTemporary();
        if (temp == 0xFF) {
            throw std::runtime_error("Out of spill memory for temporary");
        }
        
        // Format: LD Vx, value
        std::string reg_name = (temp < 15) ? "V" + std::to_string(temp) : "SPILL";
        emitInstruction(instructions, "LD", {reg_name, lit->value}, "Load literal");
        return temp;
        
    } else if (auto ident = std::dynamic_pointer_cast<Identifier>(expr)) {
        // Load variable from register or spill memory
        // (Or it could be a sprite name used as a function argument - handle both)
        if (reg_allocator_.isUserVariable(ident->name)) {
            if (reg_allocator_.isInRegister(ident->name)) {
                uint8_t var_reg = reg_allocator_.getRegister(ident->name);
                // Return user variable register directly (don't free it)
                return var_reg;
            } else if (reg_allocator_.isSpilled(ident->name)) {
                // Load from spill memory into a temporary
                uint8_t temp = reg_allocator_.allocateTemporary();
                if (temp == 0xFF) {
                    throw std::runtime_error("Out of spill memory for temporary");
                }
                uint16_t spill_addr = reg_allocator_.getSpillAddress(ident->name);
                
                std::string reg_name = "V" + std::to_string(temp);
                emitInstruction(instructions, "LD", {reg_name, "0x" + std::to_string(spill_addr)}, 
                              "Load spilled variable: " + ident->name);
                return temp;
            }
        } else {
            // Might be a sprite reference or function argument
            // For MVP, just allocate a temp and emit a placeholder load
            uint8_t temp = reg_allocator_.allocateTemporary();
            if (temp == 0xFF) {
                throw std::runtime_error("Out of spill memory for temporary");
            }
            
            std::string reg_name = "V" + std::to_string(temp);
            emitInstruction(instructions, "LD", {reg_name, ident->name}, 
                          "Load identifier (sprite or constant): " + ident->name);
            return temp;
        }
        
        throw std::runtime_error("Variable not allocated: " + ident->name);
        
    } else if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        // Evaluate left and right
        uint8_t left_reg = generateExpressionAssembly(binop->left, instructions);
        if (left_reg == 0xFF) throw std::runtime_error("Failed to evaluate left operand");
        
        uint8_t right_reg = generateExpressionAssembly(binop->right, instructions);
        if (right_reg == 0xFF) throw std::runtime_error("Failed to evaluate right operand");
        
        // Categorize the operation
        auto category = op_mapper_.categorizeOp(binop->op);
        
        if (category == OpCodeMapper::SynthesisCategory::UNSUPPORTED) {
            throw std::runtime_error("Unsupported operation: " + binop->op);
        } else if (category == OpCodeMapper::SynthesisCategory::DIRECT) {
            // Direct CHIP-8 opcode: ADD Vx, Vy → result in Vx
            std::string mnemonic = op_mapper_.mapBinaryOp(binop->op);
            std::string left_name = "V" + std::to_string(left_reg);
            std::string right_name = "V" + std::to_string(right_reg);
            
            emitInstruction(instructions, mnemonic, {left_name, right_name}, 
                          "Binary op: " + binop->op);
            
            // Free right only if it's a temporary (not a user variable)
            if (reg_allocator_.isTemporaryInRegister(right_reg)) {
                reg_allocator_.freeTemporary(right_reg);
            }
            return left_reg;
            
        } else if (category == OpCodeMapper::SynthesisCategory::COMPARISON ||
                   category == OpCodeMapper::SynthesisCategory::LOGICAL) {
            // Synthesis needed - emit synthesized code
            if (category == OpCodeMapper::SynthesisCategory::COMPARISON) {
                uint8_t result = synthesizeComparison(binop->op, left_reg, right_reg, instructions);
                
                // Free temporaries (but not user variables)
                if (reg_allocator_.isTemporaryInRegister(left_reg)) {
                    reg_allocator_.freeTemporary(left_reg);
                }
                if (reg_allocator_.isTemporaryInRegister(right_reg)) {
                    reg_allocator_.freeTemporary(right_reg);
                }
                
                return result;
            } else {
                // Logical ops (&& and ||) - for MVP, treat as comparisons
                // (Proper short-circuit would require label jumping)
                throw std::runtime_error("Logical operators (&&, ||) not yet implemented in MVP");
            }
        }
        
    } else if (auto unary = std::dynamic_pointer_cast<UnaryOp>(expr)) {
        // Unary operations: !, -, etc.
        uint8_t operand_reg = generateExpressionAssembly(unary->operand, instructions);
        if (operand_reg == 0xFF) {
            throw std::runtime_error("Failed to evaluate unary operand");
        }
        
        uint8_t result = synthesizeUnaryOp(unary->op, operand_reg, instructions);
        
        // Free operand if it's a temporary (but not a user variable)
        if (reg_allocator_.isTemporaryInRegister(operand_reg) && result != operand_reg) {
            reg_allocator_.freeTemporary(operand_reg);
        }
        
        return result;
        
    } else if (auto call = std::dynamic_pointer_cast<FunctionCall>(expr)) {
        // Check if it's a flag reader
        if (op_mapper_.isFlagReader(call->name)) {
            // Load flag from VF into a temporary
            uint8_t temp = reg_allocator_.allocateTemporary();
            if (temp == 0xFF) {
                throw std::runtime_error("Out of spill memory for temporary");
            }
            
            std::string reg_name = "V" + std::to_string(temp);
            emitInstruction(instructions, "LD", {reg_name, "VF"}, 
                          "Read flag: " + call->name);
            return temp;
        } else if (isBuiltinFunction(call->name)) {
            // Built-in function - handle specially
            generateBuiltinFunctionCall(call->name, call->args, instructions);
            return 0xFF;  // Built-ins don't return values in MVP
        } else {
            // Regular user function call
            for (const auto& arg : call->args) {
                uint8_t arg_reg = generateExpressionAssembly(arg, instructions);
                if (arg_reg != 0xFF && reg_allocator_.isTemporaryInRegister(arg_reg)) {
                    reg_allocator_.freeTemporary(arg_reg);
                }
            }
            emitInstruction(instructions, "CALL", {call->name}, "Function call: " + call->name);
            return 0xFF;  // Function calls don't return a value in MVP
        }
    }
    
    return 0xFF;  // Unknown expression type
}

// Helper: Parse register (V0-VF, DT, ST, etc.)
static uint8_t parseRegister(const std::string& reg) {
    if (reg.length() == 2 && reg[0] == 'V') {
        char c = reg[1];
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    }
    if (reg == "DT") return 0;   // Special: not a real register
    if (reg == "ST") return 0;   // Special: not a real register
    if (reg == "I") return 0;    // Special: not a real register
    throw std::runtime_error("Invalid register: " + reg);
}

// Helper: Check if a string is a literal (starts with digit or 0x)
static bool isLiteral(const std::string& s) {
    return (s[0] >= '0' && s[0] <= '9') || s.substr(0, 2) == "0x" || s.substr(0, 2) == "0X";
}

// Helper: Parse literal (hex or decimal)
static uint16_t parseLiteral(const std::string& lit) {
    if (lit.substr(0, 2) == "0x" || lit.substr(0, 2) == "0X") {
        return std::stoul(lit, nullptr, 16);
    }
    return std::stoul(lit, nullptr, 10);
}

uint16_t CodeGenerator::resolveOperand(const std::string& operand) const {
    // If it's a literal, parse it directly
    if (isLiteral(operand)) {
        return parseLiteral(operand);
    }
    
    // Otherwise, look it up in the symbol table
    uint16_t addr;
    if (symbol_table_.resolve(operand, addr)) {
        return addr;
    }
    
    throw std::runtime_error("Unresolved symbol: " + operand);
}

bool CodeGenerator::encodeInstruction(const Instruction& instr, uint16_t& out_opcode) const {
    const auto& mnemonic = instr.mnemonic;
    const auto& ops = instr.operands;
    
    try {
        if (mnemonic == "LABEL" || mnemonic == "END") {
            // LABEL and END are pseudo-instructions, not encoded
            out_opcode = 0x0000;
            return true;
        }
        
        // ===== LOAD / STORE =====
        else if (mnemonic == "LD") {
            if (ops[0] == "I") {
                // LD I, NNN → 0xAnnn (can be literal or symbol)
                uint16_t addr = resolveOperand(ops[1]);
                out_opcode = 0xA000 | (addr & 0xFFF);
                return true;
            }
            else if (ops[0] == "DT") {
                // LD DT, Vx → 0xFx15
                uint8_t x = parseRegister(ops[1]);
                out_opcode = 0xF015 | (x << 8);
                return true;
            }
            else if (ops[0] == "ST") {
                // LD ST, Vx → 0xFx18
                uint8_t x = parseRegister(ops[1]);
                out_opcode = 0xF018 | (x << 8);
                return true;
            }
            else if (ops[1] == "DT") {
                // LD Vx, DT → 0xFx07
                uint8_t x = parseRegister(ops[0]);
                out_opcode = 0xF007 | (x << 8);
                return true;
            }
            else if (ops[1] == "[I]") {
                // LD Vx, [I] → 0xFx65
                uint8_t x = parseRegister(ops[0]);
                out_opcode = 0xF065 | (x << 8);
                return true;
            }
            else if (ops[0] == "[I]") {
                // LD [I], Vx → 0xFx55
                uint8_t x = parseRegister(ops[1]);
                out_opcode = 0xF055 | (x << 8);
                return true;
            }
            else if (ops[1][0] == 'V') {
                // LD Vx, Vy → 0x8xy0
                uint8_t x = parseRegister(ops[0]);
                uint8_t y = parseRegister(ops[1]);
                out_opcode = 0x8000 | (x << 8) | (y << 4) | 0x0;
                return true;
            }
            else {
                // LD Vx, NN → 0x6xNN
                uint8_t x = parseRegister(ops[0]);
                uint8_t nn = parseLiteral(ops[1]);
                out_opcode = 0x6000 | (x << 8) | nn;
                return true;
            }
        }
        
        // ===== ARITHMETIC =====
        else if (mnemonic == "ADD") {
            if (ops[0] == "I") {
                // ADD I, Vx → 0xFx1E
                uint8_t x = parseRegister(ops[1]);
                out_opcode = 0xF01E | (x << 8);
                return true;
            }
            else if (ops[1][0] == 'V') {
                // ADD Vx, Vy → 0x8xy4
                uint8_t x = parseRegister(ops[0]);
                uint8_t y = parseRegister(ops[1]);
                out_opcode = 0x8004 | (x << 8) | (y << 4);
                return true;
            }
            else {
                // ADD Vx, NN → 0x7xNN
                uint8_t x = parseRegister(ops[0]);
                uint8_t nn = parseLiteral(ops[1]);
                out_opcode = 0x7000 | (x << 8) | nn;
                return true;
            }
        }
        else if (mnemonic == "SUB") {
            // SUB Vx, Vy → 0x8xy5
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x8005 | (x << 8) | (y << 4);
            return true;
        }
        
        // ===== BITWISE =====
        else if (mnemonic == "AND") {
            // AND Vx, Vy → 0x8xy2
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x8002 | (x << 8) | (y << 4);
            return true;
        }
        else if (mnemonic == "OR") {
            // OR Vx, Vy → 0x8xy1
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x8001 | (x << 8) | (y << 4);
            return true;
        }
        else if (mnemonic == "XOR") {
            // XOR Vx, Vy → 0x8xy3
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x8003 | (x << 8) | (y << 4);
            return true;
        }
        else if (mnemonic == "SHL") {
            // SHL Vx, Vy → 0x8xyE
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x800E | (x << 8) | (y << 4);
            return true;
        }
        else if (mnemonic == "SHR") {
            // SHR Vx, Vy → 0x8xy6
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            out_opcode = 0x8006 | (x << 8) | (y << 4);
            return true;
        }
        
        // ===== CONTROL FLOW =====
        else if (mnemonic == "JUMP") {
            // JUMP NNN → 0x1nnn (can be literal or symbol)
            uint16_t addr = resolveOperand(ops[0]);
            out_opcode = 0x1000 | (addr & 0xFFF);
            return true;
        }
        else if (mnemonic == "CALL") {
            // CALL NNN → 0x2nnn (can be literal or symbol)
            uint16_t addr = resolveOperand(ops[0]);
            out_opcode = 0x2000 | (addr & 0xFFF);
            return true;
        }
        else if (mnemonic == "RET") {
            // RET → 0x00EE
            out_opcode = 0x00EE;
            return true;
        }
        else if (mnemonic == "SE") {
            // Skip if equal
            if (ops[1][0] == 'V') {
                // SE Vx, Vy → 0x5xy0
                uint8_t x = parseRegister(ops[0]);
                uint8_t y = parseRegister(ops[1]);
                out_opcode = 0x5000 | (x << 8) | (y << 4);
            } else {
                // SE Vx, NN → 0x3xNN
                uint8_t x = parseRegister(ops[0]);
                uint8_t nn = parseLiteral(ops[1]);
                out_opcode = 0x3000 | (x << 8) | nn;
            }
            return true;
        }
        else if (mnemonic == "SNE") {
            // Skip if not equal
            if (ops[1][0] == 'V') {
                // SNE Vx, Vy → 0x9xy0
                uint8_t x = parseRegister(ops[0]);
                uint8_t y = parseRegister(ops[1]);
                out_opcode = 0x9000 | (x << 8) | (y << 4);
            } else {
                // SNE Vx, NN → 0x4xNN
                uint8_t x = parseRegister(ops[0]);
                uint8_t nn = parseLiteral(ops[1]);
                out_opcode = 0x4000 | (x << 8) | nn;
            }
            return true;
        }
        
        // ===== DISPLAY =====
        else if (mnemonic == "CLS") {
            // CLS → 0x00E0
            out_opcode = 0x00E0;
            return true;
        }
        else if (mnemonic == "DRW") {
            // DRW Vx, Vy, N → 0xDxyN
            uint8_t x = parseRegister(ops[0]);
            uint8_t y = parseRegister(ops[1]);
            uint8_t n = parseLiteral(ops[2]);
            out_opcode = 0xD000 | (x << 8) | (y << 4) | (n & 0xF);
            return true;
        }
        
        else {
            throw std::runtime_error("Unknown mnemonic: " + mnemonic);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to encode instruction '" + mnemonic + "': " + std::string(e.what()));
    }
}

namespace {

std::string trimListingSource(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) {
        start++;
    }
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end--;
    }
    std::string out = s.substr(start, end - start);
    if (out.size() > 72) {
        out.resize(72);
    }
    return out;
}

std::string formatListingAddr(uint16_t a) {
    std::ostringstream o;
    o << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << a;
    return o.str();
}

std::string formatOpcodeBytes(uint16_t opc) {
    std::ostringstream o;
    o << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << ((opc >> 8) & 0xFF);
    o << ' ';
    o << std::setw(2) << (opc & 0xFF);
    return o.str();
}

/// Text stored in the ROM debug map at each PC: prefer source line, else IR.
std::string debugMapTextForInstruction(const Instruction& instr,
                                       const std::vector<std::string>& source_lines) {
    if (!source_lines.empty() && instr.source_line > 0 &&
        static_cast<std::size_t>(instr.source_line) <= source_lines.size()) {
        std::string s = trimListingSource(source_lines[static_cast<std::size_t>(instr.source_line) - 1]);
        if (!s.empty()) {
            return s;
        }
    }
    return instr.toString();
}

} // namespace

void CodeGenerator::writeListing(const std::vector<Instruction>& instructions,
                                 const std::vector<std::string>& source_lines,
                                 std::ostream& out) const {
    const std::ios_base::fmtflags saved_flags = out.flags();

    auto sourceFor = [&](const Instruction& instr) -> std::string {
        if (source_lines.empty() || instr.source_line <= 0 ||
            static_cast<size_t>(instr.source_line) > source_lines.size()) {
            return {};
        }
        return trimListingSource(source_lines[static_cast<size_t>(instr.source_line) - 1]);
    };

    uint16_t addr = 0x200;
    for (const auto& instr : instructions) {
        if (instr.mnemonic == "LABEL") {
            out << formatListingAddr(addr) << "   --            " << std::left << std::setw(36)
                << std::setfill(' ') << instr.toString() << std::right << std::setfill(' ') << ' '
                << sourceFor(instr) << '\n';
            continue;
        }
        if (instr.mnemonic == "END") {
            out << formatListingAddr(addr) << "   --            " << std::left << std::setw(36)
                << instr.toString() << std::right << ' ' << sourceFor(instr) << '\n';
            continue;
        }

        uint16_t opc = 0;
        if (!encodeInstruction(instr, opc)) {
            throw std::runtime_error("listing: cannot encode " + instr.toString());
        }
        out << formatListingAddr(addr) << "   " << formatOpcodeBytes(opc) << "      ";

        {
            std::ostringstream mn;
            mn << std::left << std::setw(36) << std::setfill(' ') << instr.toString();
            out << mn.str();
        }
        out << ' ' << sourceFor(instr) << '\n';
        addr += 2;
    }

    if (!sprite_alloc_.sprites.empty()) {
        out << "; ROM data (sprites)\n";
        for (const auto& sprite : sprite_alloc_.sprites) {
            for (const auto& byte_str : sprite.bytes) {
                uint8_t b = static_cast<uint8_t>(parseLiteral(byte_str) & 0xFF);
                std::ostringstream db;
                db << "db 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(b);
                std::string dbstr = db.str();
                out << formatListingAddr(addr) << "   " << std::setw(5) << std::setfill(' ')
                    << std::left << std::setw(5) << ""
                    << " " << std::setw(36) << std::setfill(' ') << std::left << dbstr
                    << "; sprite " << sprite.name << '\n';
                addr += 1;
            }
        }
    }

    out.flags(saved_flags);
}

bool CodeGenerator::collectDebugMapEntries(const std::vector<Instruction>& instructions,
                                           const std::vector<std::string>& source_lines,
                                           std::vector<std::pair<std::uint16_t, std::string>>& out_entries,
                                           std::string* error_out) const {
    out_entries.clear();
    std::uint16_t addr = 0x200;
    for (const auto& instr : instructions) {
        if (instr.mnemonic == "LABEL" || instr.mnemonic == "END") {
            continue;
        }
        std::uint16_t opc = 0;
        if (!encodeInstruction(instr, opc)) {
            if (error_out) {
                *error_out = "collectDebugMapEntries: failed to encode: " + instr.toString();
            }
            return false;
        }
        out_entries.emplace_back(addr, debugMapTextForInstruction(instr, source_lines));
        addr += 2;
    }
    return true;
}

void CodeGenerator::setError(const std::string& message) {
    last_error_ = message;
}

void CodeGenerator::collectLabels(const std::vector<Instruction>& instructions) {
    // First scan: Walk through instructions, record every LABEL's address
    uint16_t current_address = 0x200;
    
    for (const auto& instr : instructions) {
        if (instr.mnemonic == "LABEL") {
            // Found a label - record its address
            if (instr.operands.empty()) {
                throw std::runtime_error("LABEL instruction missing operand (label name)");
            }
            
            const std::string& label_name = instr.operands[0];
            
            // Define the label - will throw if duplicate
            symbol_table_.define("label", label_name, current_address);
        } else if (instr.mnemonic == "END") {
            // Marker only; matches assembleToBytes (no ROM bytes emitted)
            continue;
        } else {
            // All emitted instructions are 2 bytes in CHIP-8
            current_address += 2;
        }
    }
    
    // Record where code ends (sprites start after this)
    uint16_t code_end = current_address;
    
    // === PASS 2.2: Sprite Allocation ===
    // Allocate ROM for sprites after code
    for (const auto& sprite : sprite_alloc_.sprites) {
        // Define sprite address in symbol table
        if (!symbol_table_.define("sprite", sprite.name, code_end)) {
            throw std::runtime_error("Failed to define sprite: " + sprite.name);
        }
        
        // Move to next sprite address
        code_end += sprite.bytes.size();
    }
    
    // Verify we don't overflow into stack region (0xE00)
    if (code_end >= 0xE00) {
        throw std::runtime_error("ROM too large: code + sprites exceed 0xE00");
    }
}
