#include "memory.h"

#include <fstream>
#include <iostream>
#include <iomanip>

Chip8Memory::Chip8Memory(){}

void Chip8Memory::setByte(uint16_t addr, uint8_t value) {
    ram[addr] = value;   
}

uint8_t Chip8Memory::readByte(uint16_t addr) {
    return ram[addr];
}

uint16_t Chip8Memory::readWord(uint16_t addr) {
    return (static_cast<uint16_t>(ram[addr]) << 8) | ram[addr + 1];
}


uint8_t* Chip8Memory::raw() {
    return ram.data();
}


void Chip8Memory::loadRom(const std::string &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Could not open ROM");
    }

    std::streamsize size = file.tellg();
    if (size < 0) {
        throw std::runtime_error("tellg() failed");
    }

    if (PROGRAM_START + size > MEMORY_SIZE) {
        throw std::runtime_error("ROM too big for CHIP-8 memory");
    }

    file.seekg(0);

    file.read(reinterpret_cast<char*>(&ram[PROGRAM_START]), size);
    if (!file) {
        throw std::runtime_error("ROM read failure");
    }

}
