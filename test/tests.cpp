#include <catch2/catch.hpp>

#include "chip8/InstructionSet.h"

namespace chip8_tests {
    using namespace std::literals::string_view_literals;
    TEST_CASE("translate to assembler")
    {
        auto opcode = uint16_t{ 0x8235 };
        auto assembler = chip8::opcode_to_assembler(opcode);
        REQUIRE(assembler == "SUB Vx, Vy"sv);
    }
}
