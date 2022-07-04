#ifndef CHIP8_INSTRUCTIONSET_H
#define CHIP8_INSTRUCTIONSET_H


#include <array>
#include <string_view>
#include "utilities/Map.h"

namespace chip8 {

/**
 *
 *
 * @param opcode a Chip8 opcode
 * @return The corresponding assembly instruction or "Invalid opcode".
*/
constexpr std::string_view opcode_to_assembler(uint16_t
opcode);


///////////////////////////////////////////////////////////////////////////
/// Implementation

// Masks to set the variable nibbles of opcodes to zero.
// The most significant nibble is used to index into the array.
static constexpr auto masks = std::array<uint16_t, 16>{
    0xFFFF, 0xF000, 0xF000, 0xF000,
    0xF000, 0xF000, 0xF000, 0xF000,
    0xF00F, 0xF000, 0xF000, 0xF000,
    0xF000, 0xF000, 0xF0FF, 0xF0FF
};

constexpr std::string_view opcode_to_assembler(uint16_t opcode) {
    const uint16_t mask = masks[(opcode >> 12) & 0xFU];
    switch (opcode & mask) {
    case 0x00E0: return "CLS";
    case 0x00EE: return "RET";
    case 0x1000: return "JP addr";
    case 0x2000: return "CALL addr";
    case 0x3000: return "SE Vx, byte";
    case 0x4000: return "SNE Vx, byte";
    case 0x5000: return "SE Vx, Vy";
    case 0x6000: return "LD Vx, byte";
    case 0x7000: return "ADD Vx, byte";
    case 0x8000: return "LD Vx, Vy";
    case 0x8001: return "OR Vx, Vy";
    case 0x8002: return "AND Vx, Vy";
    case 0x8003: return "XOR Vx, Vy";
    case 0x8004: return "ADD Vx, Vy";
    case 0x8005: return "SUB Vx, Vy";
    case 0x8006: return "SHR Vx {, Vy}";
    case 0x8007: return "SUBN Vx, Vy";
    case 0x800E: return "SHL Vx {, Vy}";
    case 0x9000: return "SNE Vx, Vy";
    case 0xA000: return "LD I, addr";
    case 0xB000: return "JP V0, addr";
    case 0xC000: return "RND Vx, byte";
    case 0xD000: return "DRW Vx, Vy, nibble";
    case 0xE09E: return "SKP Vx";
    case 0xE0A1: return "SKNP Vx";
    case 0xF007: return "LD Vx, DT";
    case 0xF00A: return "LD Vx, K";
    case 0xF015: return "LD DT, Vx";
    case 0xF018: return "LD ST, Vx";
    case 0xF01E: return "ADD I, Vx";
    case 0xF029: return "LD F, Vx";
    case 0xF033: return "LD B, Vx";
    case 0xF055: return "LD [I], Vx";
    case 0xF065: return "LD Vx, [I]";
    default: return "Invalid opcode";
}
}

}

#endif// CHIP8_INSTRUCTIONSET_H