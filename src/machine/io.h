#ifndef CHIP8_IO_H
#define CHIP8_IO_H

#include <cstdint>

/// Current state of the four host debug keys the runner cares about. POD; the
/// runner edge-detects and dispatches debugger commands. Frontends without a
/// keyboard (e.g. headless or pure-GUI) leave everything false.
struct HostDebugKeys {
    bool space = false;
    bool enter = false;
    bool p = false;
    bool n = false;
};

class Chip8IO {
public:
    /// CHIP-8 display is a fixed monochrome 64x32 grid. Every frontend renders
    /// at this resolution (scaled up to the host window).
    static constexpr int DISPLAY_WIDTH = 64;
    static constexpr int DISPLAY_HEIGHT = 32;

    virtual ~Chip8IO() = default;

    virtual void clearDisplay() = 0;
    virtual bool drawSprite(int x, int y, const uint8_t* sprite, int height) = 0;
    virtual void render() = 0;
    virtual void pollEvents() = 0;

    virtual bool shouldClose() const = 0;
    /// CHIP-8 keypad only: `key` in 0x0–0xF.
    virtual bool isKeyPressed(int key) const = 0;

    /// Snapshot of the host-side debug keys. Default returns all false so
    /// frontends without a keyboard need no override.
    virtual HostDebugKeys readHostDebugKeys() const { return {}; }
};


#endif
