#ifndef CHIP8_KEYPAD_H
#define CHIP8_KEYPAD_H

class Chip8Keypad {
public:
    virtual ~Chip8Keypad() = default;

    /// `key` in 0x0–0xF.
    virtual bool isKeyPressed(int key) const = 0;
};

#endif
