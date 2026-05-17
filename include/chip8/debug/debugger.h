#ifndef CHIP8_DEBUGGER_H
#define CHIP8_DEBUGGER_H

#include <chip8/debug/debug_frame.h>

#include <chrono>
#include <cstdint>
#include <deque>
#include <unordered_set>

class Chip8CPU;
class Chip8Memory;

namespace chip8::debug_map {
class DebugMap;
}

/// Owns debug state and pacing: pause/resume, single-step, step-over, breakpoints.
/// Frontends call `requestPause` / `requestResume` / `requestStep` /
/// `requestStepOver`; `Chip8Runner` pumps `tick()`.
///
/// Pacing:
///   - Auto: `tick()` self-gates with a 2 ms cadence and runs the CPU at that rate.
///           Breakpoints flip to Paused; `skip_breakpoint_once` lets a resumed run
///           execute the instruction at a breakpoint PC once.
///   - Paused: `tick()` runs one instruction per `requestStep()`, or `requestStepOver()`
///           temporarily switches to Auto until the call returns (SP-based), then
///           pauses again. `requestResume()` returns to full Auto.
class Chip8Debugger {
public:
    /// 2 ms between auto-pacing instruction executions (~500 Hz). Hand-tuned.
    static constexpr int INSTRUCTION_INTERVAL_MS = 2;
    /// Bytes of RAM around PC included in `captureFrame`.
    static constexpr std::uint16_t MEMORY_WINDOW_BYTES = 256;
    /// Ring of last executed instructions (PC + opcode before execute), for UI.
    static constexpr std::size_t INSTRUCTION_HISTORY_CAPACITY = 20;
    Chip8Debugger();

    void setBreakpoints(std::unordered_set<std::uint16_t> bps);

    void addBreakpoint(std::uint16_t pc);
    void removeBreakpoint(std::uint16_t pc);

    void requestResume();
    void requestPause();
    void requestStep();
    void requestStepOver();

    /// Self-contained snapshot frontends use to render any debug UI.
    /// When `debug_map` is non-null, fills `Chip8DebugFrame::debug_map_line` from an
    /// exact PC match (the instruction about to execute).
    Chip8DebugFrame captureFrame(const Chip8CPU& cpu, const Chip8Memory& mem,
                                 const chip8::debug_map::DebugMap* debug_map = nullptr) const;
    PausePacing pacing() const {
        return auto_pacing ? PausePacing::Auto : PausePacing::Paused;
    }
    const std::unordered_set<std::uint16_t>& getBreakpoints() const {
        return breakpoints;
    }

    /// Driven by `Chip8Runner` every loop iteration. Returns true iff an instruction
    /// was executed on this call.
    bool tick(Chip8CPU& cpu, Chip8Memory& mem);

    /// Execute exactly one instruction immediately, bypassing the timer throttle.
    /// Used by the runner's frame loop when it manages its own pacing.
    /// Returns false if paused (without a pending step) or a breakpoint fires.
    bool executeOne(Chip8CPU& cpu, Chip8Memory& mem);

    /// Debugging: get/set instruction execution speed (in Hz).
    int getInstructionSpeedHz() const { return instruction_speed_hz; }
    void setInstructionSpeedHz(int hz) {
        instruction_speed_hz = (hz > 0) ? hz : 500;
    }

    /// Reset instruction timing for frame-based execution (internal use).
    void resetInstructionTimer() {
        // Set timer to past so tick() will execute (now - past >= interval)
        int interval_ms = std::max(1, 1000 / instruction_speed_hz);
        last_instruction = clock::now() - std::chrono::milliseconds(interval_ms);
    }

private:
    using clock = std::chrono::high_resolution_clock;

    int instruction_speed_hz = 500;  // Hz instead of ms
    
    bool auto_pacing = true;
    clock::time_point last_instruction = clock::now();

    std::unordered_set<std::uint16_t> breakpoints;
    bool skip_breakpoint_once = false;

    bool step_request = false;

    bool step_over_pending = false;
    bool step_over_active = false;
    std::uint8_t step_over_target_sp = 0;

    std::deque<Chip8InstructionHistoryEntry> instruction_history_;
};

#endif
