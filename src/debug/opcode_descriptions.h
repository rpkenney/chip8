#ifndef CHIP8_OPCODE_DESCRIPTIONS_H
#define CHIP8_OPCODE_DESCRIPTIONS_H

#include <cstdint>
#include <string>

/// Returns a plain-English description of what an opcode does (one sentence or less).
std::string getOpcodeDescription(std::uint16_t opcode);

#endif
