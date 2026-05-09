#include "memory.h"

#include <cstring>
#include <stdexcept>

void Chip8Memory::setByte(uint16_t addr, uint8_t value) {
    ram[addr] = value;
}

uint8_t Chip8Memory::readByte(uint16_t addr) const {
    return ram[addr];
}

uint16_t Chip8Memory::readWord(uint16_t addr) const {
    return (static_cast<uint16_t>(ram[addr]) << 8) | ram[addr + 1];
}

void Chip8Memory::writeBytes(uint16_t addr, const uint8_t* src, std::size_t count) {
    if (static_cast<std::size_t>(addr) + count > MEMORY_SIZE) {
        throw std::out_of_range("Chip8Memory::writeBytes out of range");
    }
    std::memcpy(ram.data() + addr, src, count);
}

void Chip8Memory::readBytes(uint16_t addr, uint8_t* dst, std::size_t count) const {
    if (static_cast<std::size_t>(addr) + count > MEMORY_SIZE) {
        throw std::out_of_range("Chip8Memory::readBytes out of range");
    }
    std::memcpy(dst, ram.data() + addr, count);
}
