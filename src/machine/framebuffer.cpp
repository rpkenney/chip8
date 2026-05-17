#include <chip8/machine/framebuffer.h>

#include <cstring>

void Chip8FrameBuffer::clearDisplay() {
    std::memset(buffer_.data(), 0, buffer_.size());
}

void Chip8FrameBuffer::setPixel(int x, int y, bool active) {
    if (x < 0 || x >= Chip8Display::WIDTH || y < 0 || y >= Chip8Display::HEIGHT) {
        return;  // Out of bounds, ignore
    }
    buffer_[y * Chip8Display::WIDTH + x] = active ? 255 : 0;
}

bool Chip8FrameBuffer::getPixel(int x, int y) const {
    if (x < 0 || x >= Chip8Display::WIDTH || y < 0 || y >= Chip8Display::HEIGHT) {
        return false;  // Out of bounds
    }
    return buffer_[y * Chip8Display::WIDTH + x] != 0;
}

bool Chip8FrameBuffer::drawSprite(int x, int y, const std::uint8_t* sprite, int height) {
    bool collision = false;
    for (int i = 0; i < height; ++i) {
        std::uint8_t row = sprite[i];
        for (int j = 0; j < 8; ++j) {
            const bool spriteOn = (row & 0x80) != 0;
            row = static_cast<std::uint8_t>(row << 1);
            
            const int px = x + j;
            const int py = y + i;
            
            // Check if pixel is out of bounds
            if (px < 0 || px >= Chip8Display::WIDTH || py < 0 || py >= Chip8Display::HEIGHT) {
                // Out of bounds + sprite pixel is on = collision
                if (spriteOn) {
                    collision = true;
                }
                continue;
            }
            
            // In bounds: normal collision detection + XOR
            const bool active = buffer_[py * Chip8Display::WIDTH + px] != 0;
            collision = collision || (active && spriteOn);
            buffer_[py * Chip8Display::WIDTH + px] = (active ^ spriteOn) ? 255 : 0;
        }
    }
    return collision;
}
