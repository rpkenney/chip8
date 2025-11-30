#ifndef CHIP8_CPU_H
#define CHIP8_CPU_H

#include <cstdint>

class Chip8Memory;
class Chip8IO;

class Chip8CPU {
public:
    Chip8CPU(Chip8Memory& mem, Chip8IO& io);

    void cycle();
    void timerTick();

private:
    Chip8Memory& memory;
    Chip8IO& io;

    uint16_t pc;

    uint16_t stack[16];
    uint8_t sp;


    uint8_t reg[16];


    uint16_t I;

    uint8_t dt;
    uint8_t st;

    void executeOpcode(uint16_t opcode);
};
#endif
