#include "debug.h"

#include <cstdio>

void PrintingDebugSink::onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) {
    std::fprintf(stderr, "PC=0x%04X OPC=0x%04X\n", insn_pc, opcode);
}
