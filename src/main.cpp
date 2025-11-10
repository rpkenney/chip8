#include "io_glfw.h"
#include <iostream>

int main() {
    Chip8IO_GLFW io(640, 320, "CHIP-8");

    io.clearDisplay();
    
    while(!io.shouldClose()) {
        io.pollEvents();
        io.render();
    }

    return 0;
}
