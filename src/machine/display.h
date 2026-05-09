#ifndef CHIP8_DISPLAY_H
#define CHIP8_DISPLAY_H

#include <cstdint>

class Chip8Display {
public:
    static constexpr int WIDTH = 64;
    static constexpr int HEIGHT = 32;

    virtual ~Chip8Display() = default;

    virtual void clearDisplay() = 0;
    virtual bool drawSprite(int x, int y, const std::uint8_t* sprite, int height) = 0;
};

#endif
