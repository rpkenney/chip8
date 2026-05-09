#ifndef CHIP8_DEBUG_FRAME_H
#define CHIP8_DEBUG_FRAME_H

#include "cpu.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

enum class PausePacing { Auto, Manual };

/// Self-contained snapshot of debugger + CPU + a slice of memory at a moment in time.
/// Frontends call `Chip8Debugger::captureFrame(...)` and render the result however
/// they like (terminal text, ImGui panels, RPC payload, ...). No further accesses
/// to CPU/memory are required to format it.
struct Chip8DebugFrame {
    Chip8CpuSnapshot cpu;
    /// Word at `cpu.pc` (the instruction that is about to execute).
    std::uint16_t opcode = 0;
    /// `disassembleChip8(opcode)`.
    std::string mnemonic;
    /// First address covered by `memory_window`; aligned to an even byte.
    std::uint16_t memory_window_base = 0;
    /// Contiguous bytes starting at `memory_window_base`. Up to 64 bytes;
    /// shorter near the end of RAM.
    std::vector<std::uint8_t> memory_window;
    std::unordered_set<std::uint16_t> breakpoints;
    PausePacing pacing = PausePacing::Auto;
};

#endif
