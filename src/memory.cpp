#include "memory.h"

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
