#include "keypad_state.h"

void Chip8KeypadState::setKey(int key, bool down) {
    if (key >= 0 && key <= 0xF) {
        keys_[static_cast<std::size_t>(key)] = down;
    }
}

void Chip8KeypadState::clear() { keys_.fill(false); }

bool Chip8KeypadState::isKeyPressed(int key) const {
    if (key < 0 || key > 0xF) {
        return false;
    }
    return keys_[static_cast<std::size_t>(key)];
}
