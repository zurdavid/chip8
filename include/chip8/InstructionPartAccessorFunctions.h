#ifndef CHIP8_INSTRUCTIONPARTACCESSORFUNCTIONS_H
#define CHIP8_INSTRUCTIONPARTACCESSORFUNCTIONS_H

#include <cstdint>

namespace chip8 {
    // get 4 bit starting with bit-position startbit
    [[nodiscard]] constexpr uint16_t get4Bit(const uint16_t opcode, const uint16_t start_bit) {
        constexpr auto mask = uint16_t{0xFU};
        return (opcode >> start_bit) & mask;
    }

    // returns X in 0x_X__
    [[nodiscard]] constexpr uint16_t X(uint16_t opcode) {
        return get4Bit(opcode, 8);
    }

    // returns X in 0x__Y_
    [[nodiscard]] constexpr uint16_t Y(uint16_t opcode) {
        return get4Bit(opcode, 4);
    }

    // returns X in 0x___N
    [[nodiscard]] constexpr uint16_t n(uint16_t opcode) {
        return get4Bit(opcode, 0);
    }

    // returns X in 0x__NN
    [[nodiscard]] constexpr uint8_t nn(uint16_t opcode) {
        return opcode & 0xFFU;
    }

    // returns X in 0x_NNN
    [[nodiscard]] constexpr uint16_t nnn(uint16_t opcode) {
        return opcode & 0xFFF;
    }
}

#endif //CHIP8_INSTRUCTIONPARTACCESSORFUNCTIONS_H
