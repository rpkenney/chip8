#include <chip8/debug/opcode_descriptions.h>

#include <cstdint>
#include <string>

std::string getOpcodeDescription(std::uint16_t opcode) {
    const std::uint8_t x = static_cast<std::uint8_t>((opcode & 0x0F00) >> 8);
    const std::uint8_t y = static_cast<std::uint8_t>((opcode & 0x00F0) >> 4);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                case 0x00E0:
                    return "Clear the display.";
                case 0x00EE:
                    return "Return from the current subroutine.";
                default:
                    return "Invalid instruction.";
            }
        case 0x1000:
            return "Jump to address NNN.";
        case 0x2000:
            return "Call subroutine at address NNN.";
        case 0x3000:
            return "Skip the next instruction if Vx equals NN.";
        case 0x4000:
            return "Skip the next instruction if Vx does not equal NN.";
        case 0x5000:
            if ((opcode & 0x000F) != 0) {
                return "Invalid instruction.";
            }
            return "Skip the next instruction if Vx equals Vy.";
        case 0x6000:
            return "Set Vx to NN.";
        case 0x7000:
            return "Add NN to Vx.";
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0:
                    return "Set Vx to Vy.";
                case 0x1:
                    return "Set Vx to Vx OR Vy.";
                case 0x2:
                    return "Set Vx to Vx AND Vy.";
                case 0x3:
                    return "Set Vx to Vx XOR Vy.";
                case 0x4:
                    return "Add Vy to Vx; set VF to 1 if carry.";
                case 0x5:
                    return "Subtract Vy from Vx; set VF to 1 if no borrow.";
                case 0x6:
                    return "Shift Vx right by 1 bit; set VF to LSB.";
                case 0x7:
                    return "Subtract Vx from Vy, store in Vx; set VF to 1 if no borrow.";
                case 0xE:
                    return "Shift Vx left by 1 bit; set VF to MSB.";
                default:
                    return "Invalid instruction.";
            }
        case 0x9000:
            if ((opcode & 0x000F) != 0) {
                return "Invalid instruction.";
            }
            return "Skip the next instruction if Vx does not equal Vy.";
        case 0xA000:
            return "Set I to NNN.";
        case 0xB000:
            return "Jump to address NNN + V0.";
        case 0xC000:
            return "Set Vx to a random byte AND NN.";
        case 0xD000:
            return "Draw sprite at Vx, Vy with height N.";
        case 0xE000:
            switch (opcode & 0x00F0) {
                case 0x0090:
                    return "Skip the next instruction if key Vx is pressed.";
                case 0x00A0:
                    return "Skip the next instruction if key Vx is not pressed.";
                default:
                    return "Invalid instruction.";
            }
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007:
                    return "Set Vx to the value of the delay timer.";
                case 0x000A:
                    return "Wait for a keypress and store it in Vx.";
                case 0x0015:
                    return "Set delay timer to Vx.";
                case 0x0018:
                    return "Set sound timer to Vx.";
                case 0x001E:
                    return "Add Vx to I.";
                case 0x0029:
                    return "Set I to the font character for Vx.";
                case 0x0033:
                    return "Store decimal digits of Vx at I, I+1, I+2.";
                case 0x0055:
                    return "Store registers V0 through Vx starting at I.";
                case 0x0065:
                    return "Read registers V0 through Vx from memory at I.";
                default:
                    return "Invalid instruction.";
            }
        default:
            return "Invalid instruction.";
    }
}
