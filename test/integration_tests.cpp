#include <catch2/catch.hpp>
#include <filesystem>

#include "chip8/Chip8.h"

namespace chip8_tests {
    namespace fs = std::filesystem;
    const auto roms_path{"test_roms"};


    TEST_CASE("load ROM from file")
    {
        chip8::Chip8 chip8;
        const auto rom = fs::path(roms_path).append("test_program.ch8");
        chip8.load_rom_from_file(rom.string());

        const auto start = chip8::Chip8::pc_start_address;
        REQUIRE(chip8.get_pc() == start);
        REQUIRE(chip8.get_memory()[start] == 0x61);
        REQUIRE(chip8.get_memory()[start + 2] == 0x62);
        REQUIRE(chip8.get_memory()[start + 0x30] == 0x6c);
    }


    TEST_CASE("execute several cycles")
    {
        chip8::Chip8 chip8;
        const auto rom = fs::path(roms_path).append("test_program.ch8");      
        chip8.load_rom_from_file(rom.string());
        chip8.toggle_pause();

        // first two operations create random numbers
        SECTION("first instruction: set V1 to 4")
        {
            chip8.exec_op_cycle();
            REQUIRE(chip8.get_registers()[1] == 0x8);
        }

        SECTION("second instruction: set V2 to 8")
        {
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            REQUIRE(chip8.get_registers()[2] == 0x8);
        }

        SECTION("third instruction: set I to 0x230")
        {
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            REQUIRE(chip8.get_i() == 0x230);
        }

        SECTION("fourth instruction: draw and erase eagle")
        {
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();
            REQUIRE(chip8.get_display_buffer()[8 * 8 + 1] == 0x6C);
            REQUIRE(chip8.get_display_buffer()[9 * 8 + 1] == 0x10);
            REQUIRE(chip8.get_display_buffer()[10 * 8 + 1] == 0x28);
            chip8.exec_op_cycle();
            REQUIRE(chip8.get_display_buffer()[8 * 8 + 1] == 0x0);
            REQUIRE(chip8.get_display_buffer()[9 * 8 + 1] == 0x0);
            REQUIRE(chip8.get_display_buffer()[10 * 8 + 1] == 0x0);
        }
    }

}
