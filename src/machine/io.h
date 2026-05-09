#ifndef CHIP8_IO_H
#define CHIP8_IO_H

#include <cstdint>

class Chip8IO {
public:
    virtual ~Chip8IO() = default;

    virtual void clearDisplay() = 0;
    virtual bool  drawSprite(int x, int y, uint8_t* sprite, int height) = 0;
    virtual void render() = 0;
    virtual void pollEvents() = 0;

    virtual bool shouldClose() const = 0;
    virtual bool isKeyPressed(int key) const = 0;
}; 


#endif
