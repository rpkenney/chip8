#include "framebuffer.h"

#include <cstring>

void Chip8FrameBuffer::clearDisplay() {
    std::memset(buffer_.data(), 0, buffer_.size());
}

void Chip8FrameBuffer::setPixel(int x, int y, bool active) {
    buffer_[y * Chip8Display::WIDTH + x] = active ? 255 : 0;
}

bool Chip8FrameBuffer::getPixel(int x, int y) const {
    return buffer_[y * Chip8Display::WIDTH + x] != 0;
}

bool Chip8FrameBuffer::drawSprite(int x, int y, const std::uint8_t* sprite, int height) {
    bool collision = false;
    for (int i = 0; i < height; ++i) {
        std::uint8_t row = sprite[i];
        for (int j = 0; j < 8; ++j) {
            const bool spriteOn = (row & 0x80) != 0;
            row = static_cast<std::uint8_t>(row << 1);
            const bool active = getPixel(x + j, y + i);
            collision = collision || (active && spriteOn);
            setPixel(x + j, y + i, active ^ spriteOn);
        }
    }
    return collision;
}
