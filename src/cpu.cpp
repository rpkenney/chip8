#include "cpu.h"
#include "memory.h"
#include "io_glfw.h"

#include <stdexcept>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cstdio>

Chip8CPU::Chip8CPU(Chip8Memory &mem, Chip8IO& io)
    : memory(mem), io(io), stack{}, reg{} {
    std::srand(std::time(nullptr));
    sp = 0;
    pc = Chip8Memory::PROGRAM_START;    
    I = 0;
    dt = 0;
    st = 0;
}

void Chip8CPU::timerTick() {
    if (dt > 0) dt--;
    if (st > 0) st--;
}

static void handleInvalidOpcode(uint16_t opcode) {
    std::cout << "invalid opcode: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << opcode << std::endl;
    throw std::runtime_error("invalid opcode");
}

void Chip8CPU::executeInstruction() {
    uint16_t opcode = memory.readWord(pc);
    pc += 2;
    uint8_t n = (opcode & 0x000F);
    uint8_t nn = (opcode & 0x00FF);
    uint16_t nnn = (opcode & 0x0FFF);


    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    std::cout << "opcode: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << opcode << std::endl;
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode)  {
                case 0x00EE:
                    pc = stack[--sp];
                    break;
                case 0x00E0:
                    io.clearDisplay();
                    break;
                default:
                    handleInvalidOpcode(opcode);
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
       	    switch (opcode & 0x000F) {
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
                        reg[15] = 0x01;
                    } else {
                        reg[15] = 0x00;
                    }
                    reg[x] = static_cast<uint8_t>(sum);
                    break;
                case 0x0005:
                    if (reg[y] > reg[x]) {
                        reg[15] = 0x00;
                    } else {
                        reg[15] = 0x01;
                    }
                    reg[x] -= reg[y];
                    break;
                case 0x0006:
                    reg[x] = reg[y] >> 1;
                    reg[15] = reg[y] & 0x01;
                    break;
                case 0x0007:
                    if(reg[x] > reg[y]) {
                        reg[15] = 0x00;
                    } else {
                        reg[15] = 0x01;
                    }
                    reg[x] = reg[y] - reg[x];
                    break;
                case 0x000E:
                    reg[x] = reg[y] << 1;
                    reg[15] = reg[y] & 0x80 >> 7;
                    break;
                default:
                    handleInvalidOpcode(opcode);
            }             
            break;
        case 0x9000:
            if (reg[x] != reg[y]) {
                pc += 2;
            }
            break;
        case 0xA000:
            I = memory.readWord(nnn);
            break;
        case 0xB000:
            pc = memory.readWord(nnn + static_cast<uint16_t>(reg[0]));
            break;
        case 0xC000:
            reg[x] = (std::rand() % 256) & nn;
            break;
        case 0xD000:
            io.drawSprite(x, y, memory.raw() + I, n);
            break;
        case 0xE000:
            switch ( opcode & 0x00F0 ) {
                case 0x0090:
                   if ( io.isKeyPressed(reg[x]) ) {
                        pc += 2;
                   } 
                   break;
                case 0x00A0:
                    if ( !io.isKeyPressed(reg[x]) ) {
                        pc += 2;
                    }
                    break;
                default:
                    handleInvalidOpcode(opcode);
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0015:
                    dt = reg[x];
                    break;
                case 0x0018:
                    st = reg[x];
                    break;
                case 0x001E:
                    I += reg[x];
                    break;
                case 0x0029:
                    I = reg[x] * 5;
                    break;
                case 0x0033:
                    memory.setByte(I, reg[x] / 100);
                    memory.setByte(I + 1, (reg[x] % 100) / 10);
                    memory.setByte(I + 2, (reg[x] % 100) % 10);
                    break;
                case 0x0055:
                    memcpy(memory.raw() + I, reg, x + 1);
                    break;
                case 0x0065:
                    memcpy(reg, memory.raw() + I, x + 1);
                    break;
                default:
                    handleInvalidOpcode(opcode);
            }
            break; 
            
    }
}
