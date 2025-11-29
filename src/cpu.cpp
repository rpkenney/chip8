#include "cpu.h"
#include "memory.h"
#include "io_glfw.h"

#include <stdexcept>

Chip8CPU::Chip8CPU(Chip8Memory &mem, Chip8IO& io)
    : memory(mem), io(io), stack{}, reg{} {
    
    sp = 0;
    
}


void Chip8CPU::cycle() {
    uint16_t opcode = memory.readWord(pc)

    pc += 2;

    executeOpcode(opcode);
}


void Chip8CPU::executeOpcode(uint16_t opcode) {
    uint16_t nn = (opcode & 0x00FF);
    uint16_t nnn = (opcode & 0x0FFF);


    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode)  {
                case 0x00EE:
                    pc = stack[--sp];
                    break;
                case 0x00E0:
                    io.clearScreen();
                    break;
                default:
                    throw std::runtime_error("invalid opcode");                    
            }
            break;
        case 0x1000:
            pc = nnn;
            break;
        case 0x2000:
            stack[sp++] = pc;
            pc = nnn;
            break;      
        case 0x3000:
            if(reg[x] == nn) {
                pc += 2;
            } 
            break;
        case 0x4000:
            if(reg[x] != nn) {
                pc += 2;
            }
            break;
        case 0x5000:
            if(reg[x] == reg[y]){
                pc += 2;
            }
            break;
        case 0x6000:
            reg[x] = nn;
            break;
        case 0x7000:
            reg[x] += nn;
            break;
        case 0x8000:
            switch (opcode 0x000F) {
                case 0x0001:
                    reg[x] = reg[x] | reg[y];
                    break;
                case 0x0002:
                    reg[x] = reg[x] & reg[y];
                    break;
                case 0x0003:
                    reg[x] = reg[x] ^ reg[y]; 
                    break;
                case 0x0004:
                    uint16_t sum;
                    sum = reg[x] + reg[y];
                    if (sum > 0xFF){
                        v[15] = 0x01;
                    } else {
                        v[15] = 0x00;
                    }
                    reg[x] = static_cast<uint8_t>(sum);
                    break;
                case 0x0005:
                    if (reg[y] > reg[x]) {
                        v[15] = 0x00;
                    } else {
                        v[15] = 0x01;
                    }
                    reg[x] -= reg[y];
                    break;
                case 0x0006:
                    reg[x] = reg[y] >> 1;
                    reg[15] = reg[y] & 0x01;
                    break;
                case 0x0007:
                    if(reg[x] > reg[y]) {
                        v[15] = 0x00;
                    } else {
                        v[15] = 0x01;
                    }
                    reg[x] = reg[y] - reg[x];
                    break;
                case 0x000E:
                    reg[x] = reg[y] << 1;
                    reg[15] = reg[y] & 0x80 >> 7;
                    break;
                default:
                    throw std::runtime_error("invalid opcode");                    
            }             
            break;
    }
}
