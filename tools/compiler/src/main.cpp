#include <chip8/compiler/lexer.h>
#include <chip8/compiler/parser.h>
#include <chip8/compiler/codegen.h>
#include <chip8/debug_map/debug_map.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <vector>

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

void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <source.c8> [-o out.bin] [-l out.lst] [--embed-debug-map] [--verbose PHASES]" << std::endl;
    std::cerr << "  --embed-debug-map  Append PC→source debug blob + C8DB footer (see chip8::debug_map)."
              << std::endl;
    std::cerr << "  PHASES: comma-separated list of phase numbers to show output for" << std::endl;
    std::cerr << "  Available phases: 1 (lex), 2 (parse), 3 (codegen-pass1), 4 (codegen-pass2)" << std::endl;
    std::cerr << "  Examples: --verbose 1,2,3 or --verbose all" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string source_file = argv[1];
    std::string output_file = "out.bin";
    std::string listing_file;
    std::set<int> verbose_phases;
    bool embed_debug_map = false;

    // Parse command-line options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "-l" && i + 1 < argc) {
            listing_file = argv[++i];
        } else if (arg == "--embed-debug-map") {
            embed_debug_map = true;
        }
        else if (arg == "--verbose" && i + 1 < argc) {
            std::string phases_str = argv[++i];
            if (phases_str == "all") {
                verbose_phases = {1, 2, 3, 4};
            } else {
                // Parse comma-separated phase numbers
                std::istringstream ss(phases_str);
                std::string phase;
                while (std::getline(ss, phase, ',')) {
                    try {
                        int p = std::stoi(phase);
                        if (p >= 1 && p <= 4) {
                            verbose_phases.insert(p);
                        }
                    } catch (...) {
                        std::cerr << "Warning: invalid phase number '" << phase << "'" << std::endl;
                    }
                }
            }
        }
    }

    std::string source = readFile(source_file);
    std::vector<std::string> source_lines;
    {
        std::istringstream src_in(source);
        std::string line;
        while (std::getline(src_in, line)) {
            source_lines.push_back(line);
        }
    }

    // === PHASE 1: Lexing ===
    if (verbose_phases.count(1)) {
        std::cout << "=== Lexing ===" << std::endl;
    }
    Lexer lexer(source);
    auto tokens = lexer.scanTokens();
    if (verbose_phases.count(1)) {
        std::cout << "✓ Generated " << tokens.size() << " tokens" << std::endl << std::endl;
    }

    // === PHASE 2: Parsing ===
    if (verbose_phases.count(2)) {
        std::cout << "=== Parsing ===" << std::endl;
    }
    Parser parser(tokens);
    auto program = parser.parse();
    if (verbose_phases.count(2)) {
        std::cout << "✓ Parse successful" << std::endl << std::endl;
    }

    // === PHASE 3: Code Generation (Pass 1: AST → Assembly) ===
    if (verbose_phases.count(3)) {
        std::cout << "=== Code Generation (Pass 1: Assembly) ===" << std::endl;
    }
    CodeGenerator codegen;
    std::vector<Instruction> instructions;
    
    if (!codegen.generateAssembly(program, instructions)) {
        std::cerr << "✗ Code generation failed: " << codegen.getLastError() << std::endl;
        return 1;
    }

    if (verbose_phases.count(3)) {
        std::cout << "✓ Generated " << instructions.size() << " instructions" << std::endl;
        std::cout << "\nAssembly:" << std::endl;
        for (size_t i = 0; i < instructions.size(); ++i) {
            std::cout << "  " << (i + 1) << ".\t" << instructions[i].toString() << std::endl;
        }
        std::cout << std::endl;
    }

    // === PHASE 4: Assembly (Pass 2: Assembly → Binary) ===
    if (verbose_phases.count(4)) {
        std::cout << "=== Code Generation (Pass 2: Assembly → Binary) ===" << std::endl;
    }
    std::vector<uint8_t> binary;
    
    if (!codegen.assembleToBytes(instructions, binary)) {
        std::cerr << "✗ Assembly failed: " << codegen.getLastError() << std::endl;
        return 1;
    }

    if (verbose_phases.count(4)) {
        const auto& symbol_table = codegen.getSymbolTable();
        std::cout << "✓ Symbol Collection:" << std::endl;
        
        // Count and print labels
        int label_count = 0;
        for (const auto& label : symbol_table.labels) {
            std::cout << "  - " << label.first << " → 0x" << std::hex << label.second << std::dec << std::endl;
            label_count++;
        }
        std::cout << "  (" << label_count << " labels resolved)" << std::endl;
        
        // Count and print sprites
        int sprite_count = 0;
        for (const auto& sprite : symbol_table.sprites) {
            std::cout << "  - sprite:" << sprite.first << " → 0x" << std::hex << sprite.second << std::dec << std::endl;
            sprite_count++;
        }
        if (sprite_count > 0) {
            std::cout << "  (" << sprite_count << " sprites allocated)" << std::endl;
        }
        
        std::cout << "✓ Assembled to " << binary.size() << " bytes" << std::endl << std::endl;
    }

    // Write binary (plain ROM or container with debug trailer)
    if (embed_debug_map) {
        std::vector<std::pair<std::uint16_t, std::string>> entries;
        std::string map_err;
        if (!codegen.collectDebugMapEntries(instructions, source_lines, entries, &map_err)) {
            std::cerr << "✗ Debug map entries failed: " << map_err << std::endl;
            return 1;
        }
        std::vector<std::uint8_t> container_file;
        if (!chip8::debug_map::build_container_file(binary, entries, container_file, &map_err)) {
            std::cerr << "✗ Container build failed: " << map_err << std::endl;
            return 1;
        }
        writeFile(output_file, container_file);
    } else {
        writeFile(output_file, binary);
    }

    // Optional annotated listing (same layout as emitted ROM)
    if (!listing_file.empty()) {
        std::ofstream lst(listing_file);
        if (!lst.is_open()) {
            std::cerr << "Error: Cannot write listing file '" << listing_file << "'" << std::endl;
            return 1;
        }
        lst << "; chip8c listing for " << source_file << "\n;\n";
        try {
            codegen.writeListing(instructions, source_lines, lst);
        } catch (const std::exception& e) {
            std::cerr << "✗ Listing failed: " << e.what() << std::endl;
            return 1;
        }
    }

    // Always print compilation summary
    // Calculate sizes based on emitted instructions (each real instruction = 2 bytes)
    // and sprite data appended after code
    size_t code_bytes = 0;
    for (const auto& instr : instructions) {
        if (instr.mnemonic != "LABEL" && instr.mnemonic != "END") {
            code_bytes += 2;  // Each real instruction is 2 bytes
        }
    }
    size_t sprite_bytes = binary.size() - code_bytes;
    
    std::cout << "=== Compilation Summary ===" << std::endl;
    std::cout << "Status: ✓ SUCCESS" << std::endl;
    std::cout << "Source file: " << source_file << std::endl;
    std::cout << "Output file: " << output_file;
    if (embed_debug_map) {
        std::cout << " (ROM + embedded PC→text map)";
    }
    std::cout << std::endl;
    if (!listing_file.empty()) {
        std::cout << "Listing file: " << listing_file << std::endl;
    }
    std::cout << "Instructions: " << instructions.size() << std::endl;
    std::cout << "Code size: " << code_bytes << " bytes" << std::endl;
    if (sprite_bytes > 0) {
        std::cout << "Sprite size: " << sprite_bytes << " bytes" << std::endl;
    }
    std::cout << "Total binary size: " << binary.size() << " bytes" << std::endl;
    std::cout << std::endl;

    return 0;
}
