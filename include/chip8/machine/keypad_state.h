#ifndef CHIP8_KEYPAD_STATE_H
#define CHIP8_KEYPAD_STATE_H

#include <chip8/machine/keypad.h>

#include <array>

class Chip8KeypadState : public Chip8Keypad {
public:
    void setKey(int key, bool down);
    void clear();

    bool isKeyPressed(int key) const override;

private:
    std::array<bool, 16> keys_{};
};

#endif
