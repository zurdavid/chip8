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
//        chip8::Chip8 chip8;

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
    chip8.load_rom("../../roms/test_program.ch8");

    REQUIRE(chip8.PC == 0x200);
    REQUIRE(chip8.memory[0x200] == 0x61);
    REQUIRE(chip8.memory[0x202] == 0x62);
    REQUIRE(chip8.memory[0x200 + 0x30] == 0x6c);
    REQUIRE(chip8.program_size == 51);
}


TEST_CASE("execute several cycles")
{
    chip8::Chip8 chip8;
    chip8.load_rom("../../roms/test_program.ch8");

    // first two operations create random numbers
    SECTION("first instruction: set V1 to 4")
    {
        chip8.exec_op_cycle();
        REQUIRE(chip8.V[1] == 0x8);
    }

    SECTION("second instruction: set V2 to 8")
    {
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        REQUIRE(chip8.V[2] == 0x8);
    }

    SECTION("third instruction: set I to 0x230")
    {
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        REQUIRE(chip8.I == 0x230);
    }

    SECTION("fourth instruction: draw and erase eagle")
    {
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        REQUIRE(chip8.display_buffer[8 * 8 + 1] == 0x6C);
        REQUIRE(chip8.display_buffer[9 * 8 + 1] == 0x10);
        REQUIRE(chip8.display_buffer[10 * 8 + 1] == 0x28);
        chip8.exec_op_cycle();
        REQUIRE(chip8.display_buffer[8 * 8 + 1] == 0x0);
        REQUIRE(chip8.display_buffer[9 * 8 + 1] == 0x0);
        REQUIRE(chip8.display_buffer[10 * 8 + 1] == 0x0);
    }

}

using namespace std::literals::string_view_literals;
TEST_CASE("translate to assembler")
{
    auto opcode = uint16_t{ 0x8235 };
    auto assembler = chip8::op_to_assembler(opcode);
    REQUIRE(assembler == "SUB Vx, Vy"sv);
}