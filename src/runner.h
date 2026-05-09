#ifndef CHIP8_RUNNER_H
#define CHIP8_RUNNER_H

class Chip8CPU;
class Chip8IO;

class Chip8Runner {
public:
    Chip8Runner(Chip8CPU& cpu, Chip8IO& io);

    void setStepMode(bool enabled);

    void run();

private:
    Chip8CPU& cpu;
    Chip8IO& io;

    bool step_mode = false;
};

#endif
