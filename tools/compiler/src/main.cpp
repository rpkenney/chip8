#include "lexer.h"
#include "parser.h"
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <source.c8> [-o out.bin]" << std::endl;
        return 1;
    }

    std::string source_file = argv[1];
    std::string source = readFile(source_file);

    // Tokenize
    std::cout << "=== Lexing ===" << std::endl;
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    std::cout << "Generated " << tokens.size() << " tokens" << std::endl << std::endl;

    // Parse
    std::cout << "=== Parsing ===" << std::endl;
    try {
        Parser parser(tokens);
        auto program = parser.parse();

        std::cout << "Parse successful!" << std::endl << std::endl;
        std::cout << "=== AST ===" << std::endl;
        std::cout << program->toString() << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Parse failed: " << e.what() << std::endl;
        return 1;
    }
}
