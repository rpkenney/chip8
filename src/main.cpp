#include "io_glfw.h"
#include "memory.h"
#include "cpu.h"

#include <iostream>
#include <chrono>

int main() {
    Chip8IO_GLFW io(640, 320, "CHIP-8");
    Chip8Memory memory;    
    Chip8CPU cpu(memory, io);

    memory.loadRom("../roms/test2");

    io.clearDisplay();
    
    auto last_frame  = std::chrono::high_resolution_clock::now();    
    auto last_instruction = last_frame;
    auto curr_time = last_frame;
    while(!io.shouldClose()) {
        curr_time = std::chrono::high_resolution_clock::now();
        if ( curr_time - last_frame > std::chrono::milliseconds(16)) {
            cpu.timerTick();
            io.render();
            last_frame = curr_time;
        }

        if (curr_time - last_instruction >= std::chrono::milliseconds(2) )  {
            cpu.executeInstruction();
            last_instruction =  curr_time;
        }
    
        io.pollEvents();
    }

    return 0;
}
