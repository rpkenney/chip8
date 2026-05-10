#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write file '" << filename << "'" << std::endl;
        exit(1);
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <source.c8> [-o out.bin]" << std::endl;
        return 1;
    }

    std::string source_file = argv[1];
    std::string output_file = "out.bin";

    // Parse command-line options
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    std::string source = readFile(source_file);

    // === PHASE 1: Lexing ===
    std::cout << "=== Lexing ===" << std::endl;
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    std::cout << "✓ Generated " << tokens.size() << " tokens" << std::endl << std::endl;

    // === PHASE 2: Parsing ===
    std::cout << "=== Parsing ===" << std::endl;
    Parser parser(tokens);
    auto program = parser.parse();
    std::cout << "✓ Parse successful" << std::endl << std::endl;

    // === PHASE 3: Code Generation (Pass 1: AST → Assembly) ===
    std::cout << "=== Code Generation (Pass 1: Assembly) ===" << std::endl;
    CodeGenerator codegen;
    std::vector<Instruction> instructions;
    
    if (!codegen.generateAssembly(program, instructions)) {
        std::cerr << "✗ Code generation failed: " << codegen.getLastError() << std::endl;
        return 1;
    }

    std::cout << "✓ Generated " << instructions.size() << " instructions" << std::endl;
    std::cout << "\nAssembly:" << std::endl;
    for (const auto& instr : instructions) {
        std::cout << "  " << instr.toString() << std::endl;
    }
    std::cout << std::endl;

    // === PHASE 4: Assembly (Pass 2: Assembly → Binary) ===
    std::cout << "=== Code Generation (Pass 2: Assembly) ===" << std::endl;
    std::vector<uint8_t> binary;
    
    if (!codegen.assembleToBytes(instructions, binary)) {
        std::cerr << "✗ Assembly failed: " << codegen.getLastError() << std::endl;
        return 1;
    }

    std::cout << "✓ Assembled to " << binary.size() << " bytes" << std::endl;

    // Write binary
    writeFile(output_file, binary);
    std::cout << "✓ Wrote " << output_file << std::endl;

    return 0;
}
