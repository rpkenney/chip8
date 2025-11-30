#ifndef CHIP8_MEMORY
#define CHIP8_MEMORY

#include <cstdint>
#include <array>

class Chip8Memory {
    public:
        Chip8Memory();
        ~Chip8Memory() = default;
        
        void setByte(uint16_t addr, uint8_t value);
        uint8_t readByte(uint16_t addr);

        uint16_t readWord(uint16_t addr);
        
        uint8_t* raw();        

    private:
        static constexpr uint16_t MEMORY_SIZE = 4096;
        static constexpr uint16_t PROGRAM_START = 0x200;

        std::array<uint8_t, MEMORY_SIZE> ram{};
};


#endif
