#include "debug.h"

#include "cpu.h"
#include "disassemble_chip8.h"
#include "memory.h"

#include <cstdint>
#include <cstdio>

namespace {

constexpr std::uint16_t kMemSize = 4096;

void printMachineInspect(const Chip8CPU& cpu, const Chip8Memory& mem, std::FILE* out) {
    const Chip8CpuSnapshot s = cpu.snapshot();

    std::fprintf(out, "--- CPU ---\n");
    for (int i = 0; i < 16; ++i) {
        std::fprintf(out, "V%X=%02X%c", i, s.v[static_cast<std::size_t>(i)],
                     i == 7 || i == 15 ? '\n' : ' ');
    }
    std::fprintf(out, "I=%04X PC=%04X SP=%02X DT=%02X ST=%02X\n", s.I, s.pc, s.sp, s.dt, s.st);
    if (s.pc + 1 < kMemSize) {
        const std::uint16_t insn = mem.readWord(s.pc);
        std::fprintf(out, "insn @ PC: %04X  %s\n", insn, disassembleChip8(insn).c_str());
    }

    std::fprintf(out, "stack:");
    if (s.sp == 0) {
        std::fprintf(out, " (empty)\n");
    } else {
        for (std::uint8_t i = 0; i < s.sp; ++i) {
            std::fprintf(out, " %04X", s.stack[i]);
        }
        std::fprintf(out, "\n");
    }

    std::fprintf(out, "--- memory @ PC (16-bit words; [ ] = instruction at PC) ---\n");
    const std::uint16_t pc = s.pc;
    std::uint16_t row_start = (pc >= 8) ? static_cast<std::uint16_t>(pc - 8) : 0;
    row_start &= static_cast<std::uint16_t>(~1);

    for (int line = 0; line < 4; ++line) {
        const std::uint16_t base =
            static_cast<std::uint16_t>(row_start + static_cast<std::uint16_t>(line * 16));
        if (base >= kMemSize) {
            break;
        }
        std::fprintf(out, "%04X:", base);
        for (int b = 0; b < 16; b += 2) {
            const std::uint16_t addr = static_cast<std::uint16_t>(base + b);
            if (addr + 1 >= kMemSize) {
                break;
            }
            const std::uint16_t w = mem.readWord(addr);
            if (addr == pc) {
                std::fprintf(out, " [%04X]", w);
            } else {
                std::fprintf(out, " %04X", w);
            }
        }
        std::fprintf(out, "\n");
    }
    std::fprintf(out, "---\n");
}

}  // namespace

void PrintingDebugSink::onInstructionExecuted(std::uint16_t insn_pc, std::uint16_t opcode) {
    std::fprintf(stderr, "PC=0x%04X OPC=0x%04X  %s\n", insn_pc, opcode,
                 disassembleChip8(opcode).c_str());
}

void PrintingDebugSink::onInspectRequest(const Chip8CPU& cpu, const Chip8Memory& mem, std::FILE* out) {
    printMachineInspect(cpu, mem, out);
}
