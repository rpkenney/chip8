#include "cpu.h"
#include "memory.h"
#include "io.h"

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

void Chip8CPU::setTrace(Chip8DebugSink* sink, TraceLevel level) {
    debug_sink = sink;
    trace_level = level;
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
    const uint16_t insn_pc = pc;
    uint16_t opcode = memory.readWord(pc);
    pc += 2;
    uint8_t n = (opcode & 0x000F);
    uint8_t nn = (opcode & 0x00FF);
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
                case 0x0000:
                    reg[x] = reg[y];
                    break;
                case 0x0001:
                    reg[x] = reg[x] | reg[y];
                    break;
                case 0x0002:
                    reg[x] = reg[x] & reg[y];
                    break;
                case 0x0003:
                    reg[x] = reg[x] ^ reg[y]; 
                    break;
                case 0x0004: {
                    const uint16_t sum =
                        static_cast<uint16_t>(reg[x]) + reg[y];
                    reg[x] = static_cast<uint8_t>(sum);
                    reg[15] = static_cast<uint8_t>(sum >> 8);
                    break;
                }
                case 0x0005: {
                    const uint8_t vx = reg[x];
                    const uint8_t vy = reg[y];
                    reg[x] = vx - vy;
                    reg[15] = (vy > vx) ? 0x00 : 0x01;
                    break;
                }
                case 0x0006: {
                    // Shift quirks exist (some interpreters use Vy as the source).
                    // For now, use Vx as the source; preserve the pre-shift bit for VF.
                    const uint8_t v = reg[x];
                    reg[x] = v >> 1;
                    reg[15] = v & 0x01;
                    break;
                }
                case 0x0007: {
                    const uint8_t vx = reg[x];
                    const uint8_t vy = reg[y];
                    reg[x] = vy - vx;
                    reg[15] = (vx > vy) ? 0x00 : 0x01;
                    break;
                }
                case 0x000E: {
                    // Shift quirks exist (some interpreters use Vy as the source).
                    // For now, use Vx as the source; preserve the pre-shift bit for VF.
                    const uint8_t v = reg[x];
                    reg[x] = v << 1;
                    reg[15] = (v & 0x80) >> 7;
                    break;
                }
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
            I = nnn;
            break;
        case 0xB000:
            pc = memory.readWord(nnn + static_cast<uint16_t>(reg[0]));
            break;
        case 0xC000:
            reg[x] = (std::rand() % 256) & nn;
            break;
        case 0xD000:
            
            io.drawSprite(reg[x], reg[y], memory.raw() + I, n);
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

    if (debug_sink && trace_enabled(trace_level, TraceLevel::Instructions)) {
        debug_sink->onInstructionExecuted(insn_pc, opcode);
    }
}
