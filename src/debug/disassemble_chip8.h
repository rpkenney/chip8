#ifndef CHIP8_DISASSEMBLE_H
#define CHIP8_DISASSEMBLE_H

#include <cstdint>
#include <string>

/// Text for one 16-bit CHIP-8 word. Matches decode rules in `cpu.cpp` (invalid → "???").
std::string disassembleChip8(std::uint16_t opcode);

#endif
