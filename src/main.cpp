#include "io_glfw.h"
#include "memory.h"
#include "cpu.h"

#include <iostream>

int main() {
    Chip8IO_GLFW io(640, 320, "CHIP-8");
    Chip8Memory memory;    
    Chip8CPU cpu;

    io.clearDisplay();
    
    while(!io.shouldClose()) {
        cpu.cycle();
        io.pollEvents();
        io.render();
    }

    return 0;
}
