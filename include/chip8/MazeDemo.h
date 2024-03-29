#ifndef CHIP8_MAZEDEMO_H
#define CHIP8_MAZEDEMO_H

#include <array>
#include <cstdint>

// Maze Chip8-Rom by David Winter
static constexpr std::array<uint8_t, 34> maze_data{{
    0xa2, 0x1e, 0xc2, 0x01, 0x32, 0x01, 0xa2, 0x1a, 0xd0, 0x14, 0x70, 0x04,
    0x30, 0x40, 0x12, 0x00, 0x60, 0x00, 0x71, 0x04, 0x31, 0x20, 0x12, 0x00,
    0x12, 0x18, 0x80, 0x40, 0x20, 0x10, 0x20, 0x40, 0x80, 0x10}};

#endif // CHIP8_MAZEDEMO_H
