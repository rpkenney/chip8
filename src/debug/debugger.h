#ifndef CHIP8_DEBUGGER_H
#define CHIP8_DEBUGGER_H

#include "debug.h"
#include "debug_frame.h"

#include <chrono>
#include <cstdint>
#include <unordered_set>

class Chip8CPU;
class Chip8Memory;

/// Owns all debug state and pacing decisions: pause/resume, single-step, step-over,
/// breakpoints, and the observer (sink). Frontends drive it through command methods
/// (requestStep, requestStepOver, ...) and the runner pumps it via `tick()`.
///
/// Pacing modes:
///   - Auto:   `tick()` self-gates with a 2 ms cadence and runs the CPU at that rate.
///             Breakpoints flip the mode to Manual; `skip_breakpoint_once` lets a
///             freshly-resumed run move past a breakpoint that sits on the current PC.
///   - Manual: `tick()` only executes when a single step has been requested.
class Chip8Debugger {
public:
    /// 2 ms between auto-pacing instruction executions (~500 Hz). Hand-tuned.
    static constexpr int INSTRUCTION_INTERVAL_MS = 2;
    /// Bytes of RAM around PC included in `captureFrame`.
    static constexpr std::uint16_t MEMORY_WINDOW_BYTES = 64;

    Chip8Debugger();

    void setObserver(Chip8DebugObserver* observer);
    /// If true, start in Manual pacing and ignore `requestResume` (matches `--step`).
    void setStartPaused(bool yes);
    void setTraceLevel(TraceLevel level) { trace_level = level; }
    void setBreakpoints(std::unordered_set<std::uint16_t> bps);

    void addBreakpoint(std::uint16_t pc);
    void removeBreakpoint(std::uint16_t pc);

    void requestStep();
    void requestStepOver();
    void requestResume();
    void requestPause();

    /// Self-contained snapshot frontends use to render any debug UI.
    Chip8DebugFrame captureFrame(const Chip8CPU& cpu, const Chip8Memory& mem) const;
    PausePacing pacing() const {
        return auto_pacing ? PausePacing::Auto : PausePacing::Manual;
    }
    const std::unordered_set<std::uint16_t>& getBreakpoints() const {
        return breakpoints;
    }

    /// Driven by `Chip8Runner` every loop iteration. Returns true iff an instruction
    /// was executed on this call.
    bool tick(Chip8CPU& cpu, Chip8Memory& mem);

private:
    using clock = std::chrono::high_resolution_clock;

    Chip8DebugObserver* observer = nullptr;
    TraceLevel trace_level = TraceLevel::Off;

    bool start_paused_cli = false;
    bool auto_pacing = true;
    clock::time_point last_instruction = clock::now();

    std::unordered_set<std::uint16_t> breakpoints;
    bool skip_breakpoint_once = false;

    bool step_request = false;

    // Step-over: see DESIGN_DEBUG.md Phase 7. `pending` is set by requestStepOver and
    // consumed at the top of the next `tick()` (where we can read SP); `active` then
    // governs the post-execute stop check until SP returns to `target_sp`.
    bool step_over_pending = false;
    bool step_over_active = false;
    std::uint8_t step_over_target_sp = 0;
};

#endif
