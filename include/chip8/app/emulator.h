#ifndef CHIP8_EMULATOR_H
#define CHIP8_EMULATOR_H

#include <chip8/machine/cpu.h>
#include <chip8/debug/debugger.h>
#include <chip8/debug_map/debug_map.h>
#include <chip8/machine/framebuffer.h>
#include <chip8/machine/keypad_state.h>
#include <chip8/machine/memory.h>
#include <chip8/app/runner.h>

#include <memory>
#include <optional>
#include <string>

/// CLI / launcher options used to configure a fresh emulator instance.
struct Chip8EmulatorConfig {
    const char* rom_path = nullptr;
    const char* breakpoints_path = nullptr;
};

/// Owns RAM, display buffer, keypad, CPU, debugger, and the 60 Hz runner.
/// Construct via `create` so ROM and breakpoint file errors surface without
/// half-initialized machine state.
class Chip8Emulator {
public:
    /// Returns nullptr on load failure; `err` holds a human-readable message.
    static std::unique_ptr<Chip8Emulator> create(const Chip8EmulatorConfig& cfg,
                                                 std::string& err);

    Chip8Memory& memory() { return memory_; }
    const Chip8Memory& memory() const { return memory_; }

    Chip8FrameBuffer& framebuffer() { return framebuffer_; }
    const Chip8FrameBuffer& framebuffer() const { return framebuffer_; }

    Chip8KeypadState& keypad() { return keypad_; }
    const Chip8KeypadState& keypad() const { return keypad_; }

    Chip8CPU& cpu() { return cpu_; }
    const Chip8CPU& cpu() const { return cpu_; }

    Chip8Debugger& debugger() { return debugger_; }
    const Chip8Debugger& debugger() const { return debugger_; }

    Chip8Runner& runner() { return runner_; }
    const Chip8Runner& runner() const { return runner_; }

    /// Present when the loaded ROM file contained a valid debug-map container trailer.
    const std::optional<chip8::debug_map::DebugMap>& debugMap() const { return debug_map_; }

private:
    Chip8Emulator() = default;
    bool load(const Chip8EmulatorConfig& cfg, std::string& err);

    Chip8Memory memory_{};
    Chip8FrameBuffer framebuffer_{};
    Chip8KeypadState keypad_{};
    Chip8CPU cpu_{memory_, framebuffer_, keypad_};
    Chip8Debugger debugger_{};
    Chip8Runner runner_{cpu_, memory_, debugger_};

    std::optional<chip8::debug_map::DebugMap> debug_map_{};
};

#endif
