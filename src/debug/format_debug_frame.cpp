#include "format_debug_frame.h"

#include "debug_frame.h"

#include <cstdio>

void formatDebugFrame(const Chip8DebugFrame& frame, std::FILE* out) {
    const auto& s = frame.cpu;

    std::fprintf(out, "--- CPU ---\n");
    for (int i = 0; i < 16; ++i) {
        std::fprintf(out, "V%X=%02X%c", i, s.v[static_cast<std::size_t>(i)],
                     i == 7 || i == 15 ? '\n' : ' ');
    }
    std::fprintf(out, "I=%04X PC=%04X SP=%02X DT=%02X ST=%02X\n", s.I, s.pc, s.sp, s.dt, s.st);
    std::fprintf(out, "insn @ PC: %04X  %s\n", frame.opcode, frame.mnemonic.c_str());

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
    const std::uint16_t base = frame.memory_window_base;
    const std::size_t bytes = frame.memory_window.size();

    // 4 rows of 16 bytes (= 8 words) each, matching the prior P-dump layout.
    for (int line = 0; line < 4; ++line) {
        const std::size_t row_off = static_cast<std::size_t>(line) * 16;
        if (row_off >= bytes) {
            break;
        }
        const std::uint16_t row_base = static_cast<std::uint16_t>(base + row_off);
        std::fprintf(out, "%04X:", row_base);
        for (int b = 0; b < 16; b += 2) {
            const std::size_t off = row_off + static_cast<std::size_t>(b);
            if (off + 1 >= bytes) {
                break;
            }
            const std::uint16_t addr = static_cast<std::uint16_t>(row_base + b);
            const std::uint16_t w = static_cast<std::uint16_t>(
                (frame.memory_window[off] << 8) | frame.memory_window[off + 1]);
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
