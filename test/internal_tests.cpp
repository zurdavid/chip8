#include <catch2/catch.hpp>
#include <filesystem>

#define private public // NOLINT (clang-diagnostic-keyword-macro, cppcoreguidelines-macro-usage)
#include "chip8/Chip8.h"

namespace chip8_tests {
    namespace fs = std::filesystem;
    const auto roms_path{"test_roms"};

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
            REQUIRE(chip8.V[0xF] == 0);
        }

        SECTION("with overflow")
        {
            chip8.V[3] = 255;
            chip8.V[7] = 3;

            chip8.op_add(0x8374);
            REQUIRE(chip8.V[3] == 2);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("add 1 to 0xFF - overflow")
        {
            chip8.V[3] = 255;
            chip8.V[7] = 1;

            const uint16_t opcode3 = 0x8374;
            chip8.op_add(opcode3);
            REQUIRE(chip8.V[3] == 0);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("add 0 to 0xFF - no overflow")
        {
            chip8.V[3] = 255;
            chip8.V[7] = 0;

            const uint16_t opcode3 = 0x8374;
            chip8.op_add(opcode3);
            REQUIRE(chip8.V[3] == 255);
            REQUIRE(chip8.V[0xF] == 0);
        }
    }

    TEST_CASE("op_sub_rev")
    {
        chip8::Chip8 chip8;
        const uint16_t opcode = 0x8377;

        SECTION("no borrow: VF == 1")
        {
            chip8.V[3] = 117;
            chip8.V[7] = 210;

            chip8.op_sub_rev(opcode);
            REQUIRE(chip8.V[3] == 93);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("with borrow: VF == 0")
        {
            chip8.V[3] = 7;
            chip8.V[7] = 5;

            chip8.op_sub_rev(opcode);
            REQUIRE(chip8.V[3] == 254);
            REQUIRE(chip8.V[0xF] == 0);
        }

        SECTION("subtract zero - no borrow: VF == 1")
        {
            chip8.V[3] = 0;
            chip8.V[7] = 33;

            chip8.op_sub_rev(opcode);
            REQUIRE(chip8.V[3] == 33);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("subtract from itself - no borrow: VF == 1")
        {
            chip8.V[3] = 42;

            chip8.op_sub_rev(0x8337);
            REQUIRE(chip8.V[3] == 0);
            REQUIRE(chip8.V[0xF] == 1);
        }
    }

    TEST_CASE("op_sub")
    {
        chip8::Chip8 chip8;

        SECTION("no borrow: VF == 1")
        {
            chip8.V[3] = 210;
            chip8.V[7] = 117;

            uint16_t opcode = 0x8375;
            chip8.op_sub(opcode);
            REQUIRE(chip8.V[3] == 93);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("with borrow: VF == 0")
        {
            chip8.V[3] = 2;
            chip8.V[7] = 4;

            uint16_t opcode = 0x8375;

            chip8.op_sub(opcode);
            REQUIRE(chip8.V[3] == 254);
            REQUIRE(chip8.V[0xF] == 0);
        }

        SECTION("subtract zero - no borrow: VF == 1")
        {
            chip8.V[3] = 2;
            chip8.V[7] = 0;

            uint16_t opcode = 0x8375;

            chip8.op_sub(opcode);
            REQUIRE(chip8.V[3] == 2);
            REQUIRE(chip8.V[0xF] == 1);
        }

        SECTION("subtract from itself - no borrow: VF == 1")
        {
            chip8.V[3] = 42;

            uint16_t opcode = 0x8335;

            chip8.op_sub(opcode);
            REQUIRE(chip8.V[3] == 0);
            REQUIRE(chip8.V[0xF] == 1);
        }
    }

    TEST_CASE("call and return from subroutine")
    {
        chip8::Chip8 chip8;

        SECTION("call subroutine")
        {
            chip8.PC = 42;
            chip8.op_call_subroutine(0x2111);
            REQUIRE(chip8.PC == 0x111);
        }

        SECTION("return from subroutine")
        {
            chip8.PC = 42;
            chip8.op_call_subroutine(0x2111);
            chip8.op_return_from_subroutine(0x00EE);
            REQUIRE(chip8.PC == 42);
        }
    }

    TEST_CASE("op_goto")
    {
        chip8::Chip8 chip8;
        chip8.PC = 42;
        chip8.op_goto(0x1234);
        REQUIRE(chip8.PC == 0x234);
    }

    TEST_CASE("op_skip_ifeq") {
        chip8::Chip8 chip8;
        chip8.PC = 42;
        chip8.V[5] = 0x71;

        SECTION("equal - skip") {
            chip8.op_skip_ifeq(0x3571);
            REQUIRE(chip8.PC == 44);
        }

        SECTION("unequal - don't skip") {
            chip8.op_skip_ifeq(0x3514);
            REQUIRE(chip8.PC == 42);
        }
    }

    TEST_CASE("op_skip_ifneq") {
        chip8::Chip8 chip8;
        chip8.PC = 42;
        chip8.V[5] = 0x71;

        SECTION("equal - don't skip") {
            chip8.op_skip_ifneq(0x4571);
            REQUIRE(chip8.PC == 42);
        }

        SECTION("unequal - skip") {
            chip8.op_skip_ifneq(0x4514);
            REQUIRE(chip8.PC == 44);
        }
    }

    TEST_CASE("op_skip_ifeq_xy") {
        chip8::Chip8 chip8;
        chip8.PC = 42;
        chip8.V[1] = 0x71;
        chip8.V[7] = 0x71;
        chip8.V[0xC] = 0x81;

        SECTION("equal - skip") {
            chip8.op_skip_ifeq_xy(0x5170);
            REQUIRE(chip8.PC == 44);
        }

        SECTION("unequal - don't skip") {
            chip8.op_skip_ifeq_xy(0x5C70);
            REQUIRE(chip8.PC == 42);
        }
    }

    TEST_CASE("op_skip_ifneq_xy") {
        chip8::Chip8 chip8;
        chip8.PC = 42;
        chip8.V[1] = 0x65;
        chip8.V[7] = 0x65;
        chip8.V[0xD] = 0x78;

        SECTION("equal - don't skip") {
            chip8.op_skip_ifneq_xy(0x9170);
            REQUIRE(chip8.PC == 42);
        }

        SECTION("unequal - skip") {
            chip8.op_skip_ifneq_xy(0x9D70);
            REQUIRE(chip8.PC == 44);
        }
    }


    TEST_CASE("ld_vx_nn") {
        chip8::Chip8 chip8;
        chip8.ld_vx_nn(0x6742);
        REQUIRE(chip8.V[7] == 0x42);
    }

    TEST_CASE("op_add_nn") {
        chip8::Chip8 chip8;
        const uint8_t val = 0x77;
        chip8.V[7] = val;
        chip8.op_add_nn(0x742);

        REQUIRE(chip8.V[7] == val + 0x42);
    }

    TEST_CASE("op_assign") {
        chip8::Chip8 chip8;
        chip8.V[7] = 0x44;
        chip8.op_assign(0x8470);

        REQUIRE(chip8.V[4] == 0x44);
    }

    TEST_CASE("op_or") {
        chip8::Chip8 chip8;

        SECTION("or different bits set") {
            chip8.V[4] = 0x0C;
            chip8.V[7] = 0x90;
            chip8.op_or(0x8471);

            REQUIRE(chip8.V[4] == 0x9C);
        }
        SECTION("or some bits set the same") {
            chip8.V[4] = 0x6F;
            chip8.V[7] = 0x57;
            chip8.op_or(0x8471);

            REQUIRE(chip8.V[4] == 0x7F);
        }
    }

    TEST_CASE("op_and") {
        chip8::Chip8 chip8;

        SECTION("and different bits set") {
            chip8.V[4] = 0x0C;
            chip8.V[7] = 0x90;
            chip8.op_and(0x8472);

            REQUIRE(chip8.V[4] == 0x00);
        }
        SECTION("and some bits set the same") {
            chip8.V[4] = 0x6F;
            chip8.V[7] = 0x57;
            chip8.op_and(0x8472);

            REQUIRE(chip8.V[4] == 0x47);
        }
    }

    TEST_CASE("op_xor") {
        chip8::Chip8 chip8;

        SECTION("xor different bits set") {
            chip8.V[4] = 0x0C;
            chip8.V[7] = 0x90;
            chip8.op_xor(0x8473);

            REQUIRE(chip8.V[4] == 0x9C);
        }
        SECTION("xor some bits set the same") {
            chip8.V[4] = 0x6F;
            chip8.V[7] = 0x57;
            chip8.op_xor(0x8473);

            REQUIRE(chip8.V[4] == 0x38);
        }
    }

    TEST_CASE("op_rshift") {
        chip8::Chip8 chip8;
        chip8.set_shift_implementation(true);

        SECTION("right shift - VY to VX") {
            chip8.V[7] = 0x0F;
            chip8.op_rshift(0x8476);

            REQUIRE(chip8.V[4] == 0x07);
            REQUIRE(chip8.V[7] == 0x0F);
        }
        SECTION("right shift - least significant bit set") {
            chip8.V[7] = 0x0F;
            chip8.V[0xF] = 0x00;
            chip8.op_rshift(0x8476);

            REQUIRE(chip8.V[0xF] == 0x1);
        }
        SECTION("right shift - least significant bit not set") {
            chip8.V[7] = 0x04;
            chip8.V[0xF] = 0x01;
            chip8.op_rshift(0x8476);

            REQUIRE(chip8.V[0xF] == 0x0);
        }
        SECTION("right shift - VX to VX") {
            chip8.set_shift_implementation(false);
            chip8.V[4] = 0x0E;
            chip8.V[7] = 0xFF;
            chip8.op_rshift(0x8476);

            REQUIRE(chip8.V[4] == 0x07);
            REQUIRE(chip8.V[7] == 0xFF);
        }
        SECTION("right shift - VX to VX - least significant bit not set") {
            chip8.set_shift_implementation(false);
            chip8.V[4] = 0x0E;
            chip8.V[0xF] = 0x01;
            chip8.op_rshift(0x8476);

            REQUIRE(chip8.V[0xF] == 0x00);
        }
    }

    TEST_CASE("op_lshift") {
        chip8::Chip8 chip8;
        chip8.set_shift_implementation(true);

        SECTION("left shift - VY to VX") {
            chip8.V[7] = 0x0F;
            chip8.op_lshift(0x847E);

            REQUIRE(chip8.V[4] == 0x1E);
            REQUIRE(chip8.V[7] == 0x0F);
        }
        SECTION("left shift - least significant bit set") {
            chip8.V[7] = 0xF8;
            chip8.V[0xF] = 0x00;
            chip8.op_lshift(0x847E);

            REQUIRE(chip8.V[0xF] == 0x1);
        }
        SECTION("left shift - least significant bit not set") {
            chip8.V[7] = 0x04;
            chip8.V[0xF] = 0x01;
            chip8.op_lshift(0x847E);

            REQUIRE(chip8.V[0xF] == 0x0);
        }
        SECTION("left shift - VX to VX") {
            chip8.set_shift_implementation(false);
            chip8.V[4] = 0x0E;
            chip8.V[7] = 0xFF;
            chip8.op_lshift(0x847E);

            REQUIRE(chip8.V[4] == 0x1C);
            REQUIRE(chip8.V[7] == 0xFF);
        }
        SECTION("left shift - VX to VX - least significant bit not set") {
            chip8.set_shift_implementation(false);
            chip8.V[4] = 0x0E;
            chip8.V[0xF] = 0x01;
            chip8.op_lshift(0x847E);

            REQUIRE(chip8.V[0xF] == 0x00);
        }
    }

    TEST_CASE("op_set_i") {
        chip8::Chip8 chip8;
        chip8.op_set_i(0xA123);

        REQUIRE(chip8.I == 0x123);
    }

    TEST_CASE("op_goto_plus_reg") {
        chip8::Chip8 chip8;
        chip8.V[0] = 0x8;
        chip8.op_goto_plus_reg(0x123);

        REQUIRE(chip8.PC == 0x12B);
    }

    TEST_CASE("op_add_to_I") {
        chip8::Chip8 chip8;
        chip8.V[7] = 0x8;
        chip8.I = 0x234;
        chip8.op_add_to_I(0xF71E);

        REQUIRE(chip8.I == 0x23C);
    }

    TEST_CASE("op_set_BCD") {
        chip8::Chip8 chip8;
        const uint16_t i = 0x1AB;
        chip8.I = i;

        SECTION("255") {
            chip8.V[7] = 255;
            chip8.op_set_BCD(0xF733);

            REQUIRE(chip8.memory[i] == 2);
            REQUIRE(chip8.memory[i+1] == 5);
            REQUIRE(chip8.memory[i+2] == 5);
        }

        SECTION("12") {
            chip8.V[7] = 79;
            chip8.op_set_BCD(0xF733);

            REQUIRE(chip8.memory[i] == 0);
            REQUIRE(chip8.memory[i+1] == 7);
            REQUIRE(chip8.memory[i+2] == 9);
        }

        SECTION("0") {
            chip8.V[7] = 0;
            chip8.op_set_BCD(0xF733);

            REQUIRE(chip8.memory[i] == 0);
            REQUIRE(chip8.memory[i+1] == 0);
            REQUIRE(chip8.memory[i+2] == 0);
        }
    }

    TEST_CASE("op_set_I_to_sprite_address") {
        chip8::Chip8 chip8;

        SECTION("zero") {
            chip8.V[0xA] = 0x0;
            chip8.op_set_I_to_sprite_address(0xFA29);

            REQUIRE(chip8.I == 0);
        }

        SECTION("below 10") {
            chip8.V[0xA] = 0x4;
            chip8.op_set_I_to_sprite_address(0xFA29);

            REQUIRE(chip8.I == 5 * 4);
        }

        SECTION("above 9") {
            chip8.V[0xA] = 0xA;
            chip8.op_set_I_to_sprite_address(0xFA29);

            REQUIRE(chip8.I == 5 * 10);
        }

        SECTION("only least significant hex digit counts") {
            chip8.V[0xA] = 0x7A;
            chip8.op_set_I_to_sprite_address(0xFA29);

            REQUIRE(chip8.I == 5 * 10);
        }
    }

    TEST_CASE("timer operations") {
        chip8::Chip8 chip8;

        SECTION("op_set_delay_timer") {
            chip8.V[0x8] = 0x14;
            chip8.op_set_delay_timer(0xF815);

            REQUIRE(chip8.delay_timer == 20);
        }

        SECTION("op_set_sound_timer") {
            chip8.V[0x8] = 0x21;
            chip8.op_set_sound_timer(0xF818);

            REQUIRE(chip8.sound_timer == 0x21);
        }

        SECTION("op_to_delay_timer") {
            chip8.delay_timer = 0x33;
            chip8.op_to_delay_timer(0xF307);

            REQUIRE(chip8.V[3] == 0x33);
        }
    }

    TEST_CASE("op_keyop1") {
        chip8::Chip8 chip8;
        const auto key = 0x3;
        chip8.V[6] = key;
        chip8.PC = 38;

        SECTION("key pressed - skip") {
            chip8.keys[key] = true;
            chip8.op_keyop1(0xE69E);

            REQUIRE(chip8.PC == 40);
        }

        SECTION("key not pressed - don't skip") {
            chip8.keys[key] = false;
            chip8.op_keyop1(0xE69E);

            REQUIRE(chip8.PC == 38);
        }
    }

    TEST_CASE("op_keyop2") {
        chip8::Chip8 chip8;
        const auto key = 0xA;
        chip8.V[2] = key;
        chip8.PC = 12;

        SECTION("key pressed - don't skip") {
            chip8.keys[key] = true;
            chip8.op_keyop2(0xE2A1);

            REQUIRE(chip8.PC == 12);
        }

        SECTION("key not pressed - skip") {
            chip8.keys[key] = false;
            chip8.op_keyop2(0xE2A1);

            REQUIRE(chip8.PC == 14);
        }
    }

    TEST_CASE("op_get_key") {
        chip8::Chip8 chip8;
        chip8.V[0xD] = 0xFF;
        chip8.PC = 12;
        std::fill(chip8.keys.begin(), chip8.keys.begin(), false);

        SECTION("no key pressed - block") {
            chip8.op_get_key(0xFD0A);

            REQUIRE(chip8.V[0xD] == 0xFF);
            REQUIRE(chip8.PC == 10);
        }

        SECTION("key 7 pressed") {
            chip8.keys[7] = true;

            chip8.op_get_key(0xFD0A);

            REQUIRE(chip8.V[0xD] == 0x07);
            REQUIRE(chip8.PC == 12);
        }

        SECTION("keys A and E pressed") {
            chip8.keys[0xE] = true;
            chip8.keys[0xA] = true;
            chip8.op_get_key(0xFD0A);

            REQUIRE(chip8.V[0xD] == 0xA);
            REQUIRE(chip8.PC == 12);
        }
    }

   TEST_CASE("op_regdump")
    {
        chip8::Chip8 chip8;

        // dump registers 0 to 0xB == 11 in memory
        // starting at address I
        const uint16_t opcode = 0xFB55;

        std::iota(chip8.V.begin(), chip8.V.end(), 101);

        chip8.I = 1000;
        chip8.op_regdump(opcode);

        REQUIRE(chip8.memory[1000] == chip8.V[0]);
        REQUIRE(chip8.memory[1000] == 101);
        REQUIRE(chip8.memory[1003] == chip8.V[3]);
        REQUIRE(chip8.memory[1011] == chip8.V[11]);
        REQUIRE(chip8.memory[1012] != chip8.V[12]);
        REQUIRE(chip8.memory[1015] != chip8.V[15]);
    }


    TEST_CASE("op_regload")
    {
        chip8::Chip8 chip8;

        // load memory into registers 0 to 0xB == 11
        // starting from address I
        const uint16_t opcode = 0xFB65;
        const uint16_t n = 12;

        auto* start = chip8.memory.begin() + 1000;
        std::iota(start, start + n, 101); // NOLINT pointer arithmetic secure

        chip8.I = 1000;
        chip8.op_regload(opcode);

        REQUIRE(chip8.memory[1000] == chip8.V[0]);
        REQUIRE(chip8.V[0] == 101);
        REQUIRE(chip8.memory[1003] == chip8.V[3]);
        REQUIRE(chip8.memory[1011] == chip8.V[11]);
        REQUIRE(chip8.V[0xB] == 101 + 0xB);
    }

    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N+1 pixels.
    // Each row of 8 pixels is read as bit-coded starting from memory location I;
    // I value does not change after the execution of this instruction.
    // As described above, VF is set to 1 if any screen pixels are flipped
    // from set to unset when the sprite is drawn, and to 0 if that does not happen
    TEST_CASE("op_draw 0xD000") {
        chip8::Chip8 chip8;
//        static constexpr std::array<uint8_t, 4> {};

        SECTION("draw_cube") {

        }
    }

    TEST_CASE("load ROM")
    {
        chip8::Chip8 chip8;
        const auto rom = fs::path(roms_path).append("test_program.ch8");
        chip8.load_rom(rom);

        REQUIRE(chip8.PC == 0x200);
        REQUIRE(chip8.memory[0x200] == 0x61);
        REQUIRE(chip8.memory[0x202] == 0x62);
        REQUIRE(chip8.memory[0x200 + 0x30] == 0x6c);
        REQUIRE(chip8.program_size == 51);
    }


    TEST_CASE("execute several cycles")
    {
        chip8::Chip8 chip8;
        const auto rom = fs::path(roms_path).append("test_program.ch8");
        chip8.load_rom(rom);
        chip8.toggle_pause();

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

}
