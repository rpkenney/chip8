#include "disassemble_chip8.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

bool appendf(char* buf, std::size_t cap, std::size_t& len, const char* fmt, ...) {
    if (len >= cap) {
        return false;
    }
    va_list ap;
    va_start(ap, fmt);
    const int n = std::vsnprintf(buf + len, cap - len, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return false;
    }
    len += static_cast<std::size_t>(n);
    return len < cap;
}

}  // namespace

std::string disassembleChip8(std::uint16_t opcode) {
    const std::uint8_t n = static_cast<std::uint8_t>(opcode & 0x000F);
    const std::uint8_t nn = static_cast<std::uint8_t>(opcode & 0x00FF);
    const std::uint16_t nnn = static_cast<std::uint16_t>(opcode & 0x0FFF);
    const std::uint8_t x = static_cast<std::uint8_t>((opcode & 0x0F00) >> 8);
    const std::uint8_t y = static_cast<std::uint8_t>((opcode & 0x00F0) >> 4);

    char buf[96];
    std::size_t len = 0;
    const std::size_t cap = sizeof(buf);

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode) {
                case 0x00E0:
                    std::memcpy(buf, "CLS", 4);
                    return buf;
                case 0x00EE:
                    std::memcpy(buf, "RET", 4);
                    return buf;
                default:
                    return "???";
            }
        case 0x1000:
            appendf(buf, cap, len, "JP #0x%03X", nnn);
            return buf;
        case 0x2000:
            appendf(buf, cap, len, "CALL #0x%03X", nnn);
            return buf;
        case 0x3000:
            appendf(buf, cap, len, "SE V%X,#0x%02X", x, nn);
            return buf;
        case 0x4000:
            appendf(buf, cap, len, "SNE V%X,#0x%02X", x, nn);
            return buf;
        case 0x5000:
            if ((opcode & 0x000F) != 0) {
                return "???";
            }
            appendf(buf, cap, len, "SE V%X,V%X", x, y);
            return buf;
        case 0x6000:
            appendf(buf, cap, len, "LD V%X,#0x%02X", x, nn);
            return buf;
        case 0x7000:
            appendf(buf, cap, len, "ADD V%X,#0x%02X", x, nn);
            return buf;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0:
                    appendf(buf, cap, len, "LD V%X,V%X", x, y);
                    return buf;
                case 0x1:
                    appendf(buf, cap, len, "OR V%X,V%X", x, y);
                    return buf;
                case 0x2:
                    appendf(buf, cap, len, "AND V%X,V%X", x, y);
                    return buf;
                case 0x3:
                    appendf(buf, cap, len, "XOR V%X,V%X", x, y);
                    return buf;
                case 0x4:
                    appendf(buf, cap, len, "ADD V%X,V%X", x, y);
                    return buf;
                case 0x5:
                    appendf(buf, cap, len, "SUB V%X,V%X", x, y);
                    return buf;
                case 0x6:
                    appendf(buf, cap, len, "SHR V%X", x);
                    return buf;
                case 0x7:
                    appendf(buf, cap, len, "SUBN V%X,V%X", x, y);
                    return buf;
                case 0xE:
                    appendf(buf, cap, len, "SHL V%X", x);
                    return buf;
                default:
                    return "???";
            }
        case 0x9000:
            if ((opcode & 0x000F) != 0) {
                return "???";
            }
            appendf(buf, cap, len, "SNE V%X,V%X", x, y);
            return buf;
        case 0xA000:
            appendf(buf, cap, len, "LD I,#0x%03X", nnn);
            return buf;
        case 0xB000:
            // This core: PC = mem.word(nnn+V0), not JP V0+nnn.
            appendf(buf, cap, len, "JP.MEM #0x%03X+V0", nnn);
            return buf;
        case 0xC000:
            appendf(buf, cap, len, "RND V%X,#0x%02X", x, nn);
            return buf;
        case 0xD000:
            appendf(buf, cap, len, "DRW V%X,V%X,#%X", x, y, n);
            return buf;
        case 0xE000:
            switch (opcode & 0x00F0) {
                case 0x0090:
                    appendf(buf, cap, len, "SKP V%X", x);
                    return buf;
                case 0x00A0:
                    appendf(buf, cap, len, "SKNP V%X", x);
                    return buf;
                default:
                    return "???";
            }
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0015:
                    appendf(buf, cap, len, "LD DT,V%X", x);
                    return buf;
                case 0x0018:
                    appendf(buf, cap, len, "LD ST,V%X", x);
                    return buf;
                case 0x001E:
                    appendf(buf, cap, len, "ADD I,V%X", x);
                    return buf;
                case 0x0029:
                    appendf(buf, cap, len, "LD F,V%X", x);
                    return buf;
                case 0x0033:
                    appendf(buf, cap, len, "LD B,V%X", x);
                    return buf;
                case 0x0055:
                    appendf(buf, cap, len, "LD [I],V%X", x);
                    return buf;
                case 0x0065:
                    appendf(buf, cap, len, "LD V%X,[I]", x);
                    return buf;
                default:
                    return "???";
            }
        default:
            return "???";
    }
}
