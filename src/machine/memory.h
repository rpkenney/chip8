#ifndef CHIP8_MEMORY
#define CHIP8_MEMORY

#include <array>
#include <cstddef>
#include <cstdint>

class Chip8Memory {
public:
    Chip8Memory() = default;

    void setByte(uint16_t addr, uint8_t value);
    uint8_t readByte(uint16_t addr) const;
    uint16_t readWord(uint16_t addr) const;

    /// Bulk write/read. Throw `std::out_of_range` if the range falls off the end
    /// of RAM. Replaces the prior `raw()` pointer escape hatch.
    void writeBytes(uint16_t addr, const uint8_t* src, std::size_t count);
    void readBytes(uint16_t addr, uint8_t* dst, std::size_t count) const;

    static constexpr uint16_t PROGRAM_START = 0x200;
    static constexpr uint16_t MEMORY_SIZE = 4096;

private:
    std::array<uint8_t, MEMORY_SIZE> ram{};
};

#endif
