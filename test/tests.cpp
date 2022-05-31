#include <catch2/catch.hpp>
#include "chip8/InstructionSet.h"
#define private public
#include "chip8/Chip8.h"


TEST_CASE("ld_vx_nn load byt to register Vx")
{
    chip8::Chip8 chip8;

    const uint16_t opcode = 0x630C;
    chip8.ld_vx_nn(opcode);
    const uint16_t opcode2 = 0x6719;
    chip8.ld_vx_nn(opcode2);

    REQUIRE(chip8.V[3] == 0x0C);
    REQUIRE(chip8.V[7] == 0x19);
}


TEST_CASE("op_add")
{
    chip8::Chip8 chip8;

    SECTION("without overflow")
    {
        const uint16_t opcode = 0x630C;
        chip8.ld_vx_nn(opcode);
        const uint16_t opcode2 = 0x6719;
        chip8.ld_vx_nn(opcode2);

        const uint16_t opcode3 = 0x8374;
        chip8.op_add(opcode3);
        REQUIRE(chip8.V[3] == 37);
        REQUIRE(chip8.V[15] == 0);
    }
    SECTION("without overflow")
    {
        const uint16_t opcode = 0x63FF;
        chip8.ld_vx_nn(opcode);
        const uint16_t opcode2 = 0x6703;
        chip8.ld_vx_nn(opcode2);

        const uint16_t opcode3 = 0x8374;
        chip8.op_add(opcode3);
        REQUIRE(chip8.V[3] == 2);
        REQUIRE(chip8.V[15] == 1);
    }
}

TEST_CASE("op_sub")
{
    chip8::Chip8 chip8;

    SECTION("no borrow VF - 1")
    {
        chip8.V[3] = 210;
        chip8.V[7] = 117;

        uint16_t opcode = 0x8375;
        chip8.op_sub(opcode);
        REQUIRE(chip8.V[3] == 93);
        REQUIRE(chip8.V[15] == 1);
    }
    SECTION("with borrow VF - 0")
    {
        chip8::Chip8 chip8;

        chip8.V[3] = 2;
        chip8.V[7] = 4;

        uint16_t opcode = 0x8375;

        chip8.op_sub(opcode);
        REQUIRE(chip8.V[3] == 254);
        REQUIRE(chip8.V[15] == 0);
    }
}

TEST_CASE("op_regdump")
{
    chip8::Chip8 chip8;

    const uint16_t n = 12;
    const uint16_t opcode = 0xFB55;

    auto start = chip8.V.begin();
    std::iota(start, start + n, 101);

    chip8.I = 1000;
    chip8.op_regdump(opcode);

    REQUIRE(chip8.memory[1000] == chip8.V[0]);
    REQUIRE(chip8.memory[1000] == 101);
    REQUIRE(chip8.memory[1003] == chip8.V[3]);
    REQUIRE(chip8.memory[1011] == chip8.V[11]);
}


TEST_CASE("op_regload")
{
    chip8::Chip8 chip8;

    const uint16_t n = 12;
    const uint16_t opcode = 0xFB65;

    auto* start = chip8.memory.begin() + 1000;
    std::iota(start, start + n, 101);

    chip8.I = 1000;
    chip8.op_regload(opcode);

    REQUIRE(chip8.memory[1000] == chip8.V[0]);
    REQUIRE(chip8.V[0] == 101);
    REQUIRE(chip8.memory[1003] == chip8.V[3]);
    REQUIRE(chip8.memory[1011] == chip8.V[11]);
    REQUIRE(chip8.V[0xB] == 101 + 0xB);
}

TEST_CASE("load ROM")
{
    chip8::Chip8 chip8;
    chip8.load_rom("../../roms/pong.ch8");

    REQUIRE(chip8.PC == 512);
    REQUIRE(chip8.memory[0x200] == 0x6A);
    REQUIRE(chip8.memory[0x200 + 0xF0] == 0x80);
}


TEST_CASE("execute cycle")
{
    chip8::Chip8 chip8;
    chip8.load_rom("../../roms/pong.ch8");

    chip8.exec_op_cycle();

    REQUIRE(chip8.PC == 514);
    REQUIRE(chip8.V[0xA] == 0x02);
}


using namespace std::literals::string_view_literals;
TEST_CASE("some test")
{
    auto opcode = uint16_t{ 0x8235 };
    auto assembler = chip8::op_to_assembler(opcode);
    REQUIRE(assembler == "SUB Vx, Vy"sv);
}