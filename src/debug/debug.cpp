#include "debug.h"

#include "disassemble_chip8.h"

#include <cstdint>
#include <cstdio>

void TerminalDebugObserver::onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) {
    std::fprintf(stderr, "PC=0x%04X OPC=0x%04X  %s\n", insn_pc, opcode,
                 disassembleChip8(opcode).c_str());
}
