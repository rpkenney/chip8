#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H

#include <array>
#include <cstdint>
#include <random>

class Chip8Memory;
class Chip8Display;
class Chip8Keypad;

struct Chip8CpuSnapshot {
    std::array<std::uint8_t, 16> v{};
    std::uint16_t I = 0;
    std::uint16_t pc = 0;
    std::uint8_t sp = 0;
    std::array<std::uint16_t, 16> stack{};
    std::uint8_t dt = 0;
    std::uint8_t st = 0;
};

class Chip8CPU {
public:
    Chip8CPU(Chip8Memory& mem, Chip8Display& display, Chip8Keypad& keypad);

    std::uint16_t getPC() const { return pc; }
    /// Current stack depth (0 = empty). Used by the debugger for step-over.
    std::uint8_t getSP() const { return sp; }

    Chip8CpuSnapshot snapshot() const;

    /// Re-seed the per-CPU RNG used by `Cxnn` (RND). Default seed comes from
    /// `std::random_device` in the constructor; tests/repros call this to
    /// pin the sequence.
    void setSeed(std::uint32_t seed) { rng.seed(seed); }

    void executeInstruction();
    void timerTick();

private:
    Chip8Memory& memory;
    Chip8Display& display;
    Chip8Keypad& keypad;

    std::mt19937 rng;

    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t reg[16];
    uint16_t I;
    uint8_t dt;
    uint8_t st;
};
#endif
