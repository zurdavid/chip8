#ifndef CHIP8_INSTRUCTIONSET_H
#define CHIP8_INSTRUCTIONSET_H


#include <array>
#include <string_view>
#include "utilities/Map.h"

namespace chip8 {

static constexpr int N_OPCODES = 34;

static constexpr auto masks = std::array<uint16_t, 16>{
  0xFFFF, 0xF000, 0xF000, 0xF000,
  0xF000, 0xF000, 0xF000, 0xF000,
  0xF00F, 0xF000, 0xF000, 0xF000,
  0xF000, 0xF000, 0xF0FF, 0xF0FF
};

using namespace std::literals::string_view_literals;
static constexpr std::array<std::pair<uint16_t, std::string_view>, N_OPCODES> operations{ {
{ 0x00E0,  "CLS" },
{ 0x00EE,  "RET" },
{ 0x1000,  "JP addr" },
{ 0x2000,  "CALL addr" },
{ 0x3000,  "SE Vx, byte" },
{ 0x4000,  "SNE Vx, byte" },
{ 0x5000,  "SE Vx, Vy" },
{ 0x6000,  "LD Vx, byte" },
{ 0x7000,  "ADD Vx, byte" },
{ 0x8000,  "LD Vx, Vy" },
{ 0x8001,  "OR Vx, Vy" },
{ 0x8002,  "AND Vx, Vy" },
{ 0x8003,  "XOR Vx, Vy" },
{ 0x8004,  "ADD Vx, Vy" },
{ 0x8005,  "SUB Vx, Vy" },
{ 0x8006,  "SHR Vx {, Vy}" },
{ 0x8007,  "SUBN Vx, Vy" },
{ 0x800E,  "SHL Vx {, Vy}" },
{ 0x9000,  "SNE Vx, Vy" },
{ 0xA000,  "LD I, addr" },
{ 0xB000,  "JP V0, addr" },
{ 0xC000,  "RND Vx, byte" },
{ 0xD000,  "DRW Vx, Vy, nibble" },
{ 0xE09E,  "SKP Vx" },
{ 0xE0A1,  "SKNP Vx" },
{ 0xF007,  "LD Vx, DT" },
{ 0xF00A,  "LD Vx, K" },
{ 0xF015,  "LD DT, Vx" },
{ 0xF018,  "LD ST, Vx" },
{ 0xF01E,  "ADD I, Vx" },
{ 0xF029,  "LD F, Vx" },
{ 0xF033,  "LD B, Vx" },
{ 0xF055,  "LD [I], Vx" },
{ 0xF065,  "LD Vx, [I]" },
} };

std::string_view op_to_assembler(uint16_t opcode) {
    static constexpr auto map = Map<uint16_t, std::string_view, N_OPCODES>{{operations}};
    static constexpr auto error_msg = "no valid opcode"sv;
    auto mask = uint16_t{0xFU};
    mask = masks[(opcode >> 12) & mask];
    try {
        const auto assembler = map.at(opcode & mask);
        return assembler;
    } catch (const std::range_error& e) {
        return error_msg;
    }
}

}





#endif// CHIP8_INSTRUCTIONSET_H
