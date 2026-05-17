#ifndef CHIP8_FRAMEBUFFER_H
#define CHIP8_FRAMEBUFFER_H

#include <chip8/machine/display.h>

#include <array>
#include <cstddef>
#include <cstdint>

/// 64x32 monochrome framebuffer with CHIP-8 XOR/collision sprite draw.
class Chip8FrameBuffer : public Chip8Display {
public:
    static constexpr std::size_t PIXEL_COUNT =
        static_cast<std::size_t>(WIDTH) * static_cast<std::size_t>(HEIGHT);

    void clearDisplay() override;

    /// XOR sprite into buffer; returns true if a lit pixel was cleared.
    bool drawSprite(int x, int y, const std::uint8_t* sprite, int height) override;

    /// Row-major, 0 = off, 255 = on, `WIDTH * HEIGHT` bytes.
    const std::uint8_t* pixels() const { return buffer_.data(); }

private:
    void setPixel(int x, int y, bool active);
    bool getPixel(int x, int y) const;

    std::array<std::uint8_t, PIXEL_COUNT> buffer_{};
};

#endif
