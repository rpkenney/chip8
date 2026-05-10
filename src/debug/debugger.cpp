#include "debugger.h"

#include "cpu.h"
#include "disassemble_chip8.h"
#include "memory.h"
#include "opcode_descriptions.h"

#include <utility>

Chip8Debugger::Chip8Debugger() = default;

void Chip8Debugger::setBreakpoints(std::unordered_set<std::uint16_t> bps) {
    breakpoints = std::move(bps);
}

void Chip8Debugger::addBreakpoint(std::uint16_t pc) {
    breakpoints.insert(pc);
}

void Chip8Debugger::removeBreakpoint(std::uint16_t pc) {
    breakpoints.erase(pc);
}

void Chip8Debugger::requestStep() {
    if (!auto_pacing) {
        step_request = true;
    }
}

void Chip8Debugger::requestStepOver() {
    if (!auto_pacing && !step_over_active && !step_over_pending) {
        step_over_pending = true;
    }
}

void Chip8Debugger::requestResume() {
    if (!auto_pacing) {
        auto_pacing = true;
        skip_breakpoint_once = true;
    }
}

void Chip8Debugger::requestPause() {
    if (auto_pacing) {
        auto_pacing = false;
        step_over_active = false;
        step_over_pending = false;
    }
}

Chip8DebugFrame Chip8Debugger::captureFrame(const Chip8CPU& cpu,
                                            const Chip8Memory& mem) const {
    Chip8DebugFrame frame;
    frame.cpu = cpu.snapshot();
    frame.opcode = (frame.cpu.pc + 1 < Chip8Memory::MEMORY_SIZE)
                       ? mem.readWord(frame.cpu.pc) : 0;
    frame.mnemonic = disassembleChip8(frame.opcode);
    frame.description = getOpcodeDescription(frame.opcode);

    // Window centered on PC (aligned to even), clamped at 0.
    constexpr std::uint16_t kHalf = MEMORY_WINDOW_BYTES / 2;
    std::uint16_t base = (frame.cpu.pc >= kHalf) ? static_cast<std::uint16_t>(frame.cpu.pc - kHalf) : 0;
    base &= static_cast<std::uint16_t>(~1);
    frame.memory_window_base = base;
    const std::uint16_t end = (base + MEMORY_WINDOW_BYTES < Chip8Memory::MEMORY_SIZE)
                                  ? static_cast<std::uint16_t>(base + MEMORY_WINDOW_BYTES)
                                  : Chip8Memory::MEMORY_SIZE;
    frame.memory_window.reserve(end - base);
    for (std::uint16_t a = base; a < end; ++a) {
        frame.memory_window.push_back(mem.readByte(a));
    }

    frame.breakpoints = breakpoints;
    frame.pacing = pacing();

    frame.instruction_history.assign(instruction_history_.begin(),
                                       instruction_history_.end());

    return frame;
}

bool Chip8Debugger::executeOne(Chip8CPU& cpu, Chip8Memory& mem) {
    if (!auto_pacing) {
        return false;
    }
    const std::uint16_t pc = cpu.getPC();
    if (!breakpoints.empty() && breakpoints.count(pc) != 0) {
        if (skip_breakpoint_once) {
            skip_breakpoint_once = false;
        } else {
            auto_pacing = false;
            step_over_active = false;
            return false;
        }
    }

    const std::uint16_t insn_pc = cpu.getPC();
    const std::uint16_t insn_opcode =
        (insn_pc + 1 < Chip8Memory::MEMORY_SIZE) ? mem.readWord(insn_pc) : 0;

    cpu.executeInstruction();
    last_instruction = clock::now();

    if (instruction_history_.size() >= INSTRUCTION_HISTORY_CAPACITY) {
        instruction_history_.pop_front();
    }
    instruction_history_.push_back({insn_pc, insn_opcode});

    if (step_over_active && cpu.getSP() <= step_over_target_sp) {
        step_over_active = false;
        auto_pacing = false;
    }

    return true;
}

bool Chip8Debugger::tick(Chip8CPU& cpu, Chip8Memory& mem) {
    // Materialise a pending step-over now that we can read SP. We flip to auto
    // pacing internally and let the auto path drive execution until SP returns.
    if (step_over_pending) {
        step_over_pending = false;
        step_over_active = true;
        step_over_target_sp = cpu.getSP();
        auto_pacing = true;
        skip_breakpoint_once = true;
    }

    const auto now = clock::now();
    bool do_execute = false;

    if (!auto_pacing) {
        if (step_request) {
            step_request = false;
            do_execute = true;
        }
    } else if (now - last_instruction >= std::chrono::milliseconds(std::max(1, 1000 / instruction_speed_hz))) {
        const std::uint16_t pc = cpu.getPC();
        if (!breakpoints.empty() && breakpoints.count(pc) != 0) {
            if (skip_breakpoint_once) {
                skip_breakpoint_once = false;
            } else {
                auto_pacing = false;
                // Breakpoint wins over an in-flight step-over.
                step_over_active = false;
            }
        }
        if (auto_pacing) {
            do_execute = true;
        }
    }

    if (!do_execute) {
        return false;
    }

    // Snapshot insn_pc/opcode before execute so history shows the insn that ran.
    const std::uint16_t insn_pc = cpu.getPC();
    const std::uint16_t insn_opcode =
        (insn_pc + 1 < Chip8Memory::MEMORY_SIZE) ? mem.readWord(insn_pc) : 0;

    cpu.executeInstruction();
    last_instruction = now;

    if (instruction_history_.size() >= INSTRUCTION_HISTORY_CAPACITY) {
        instruction_history_.pop_front();
    }
    instruction_history_.push_back({insn_pc, insn_opcode});

    // Step-over done once SP is back at (or below) the recorded depth.
    if (step_over_active && cpu.getSP() <= step_over_target_sp) {
        step_over_active = false;
        auto_pacing = false;
    }

    return true;
}
