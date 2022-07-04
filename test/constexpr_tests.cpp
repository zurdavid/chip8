#include <catch2/catch.hpp>
#include "utilities/Map.h"
#include "chip8/InstructionSet.h"

TEST_CASE("constexpr opcodeAssemblyMap - simple", "[constexpr opcodeAssemblyMap]")
{
    static constexpr std::array<std::pair<uint16_t, std::string_view>, 4> vals{{
            {0xA, "AA"},
            {0xC, "CC"},
            {0xF, "FF"}
    }};

    static constexpr auto map = Map<uint16_t, std::string_view, 4>{{vals}};

    SECTION("treffer") {
        STATIC_REQUIRE(map.maybeAt(0xC) == "CC");
    }
    SECTION("treffer3") {
        STATIC_REQUIRE(map.at(0xA) == "AA");
    }
    SECTION("naught") {
        STATIC_REQUIRE(map.maybeAt(0x1) == std::nullopt);
    }
}

TEST_CASE("opcode to assembly instruction") {
    SECTION("valid opcode variable nibbles set to 0") {
        static constexpr uint16_t opcode = 0x8007;
        static constexpr auto expected = "SUBN Vx, Vy";
        STATIC_REQUIRE(chip8::opcode_to_assembler(opcode) == expected);
    }

    SECTION("valid opcode variable nibbles set to 0") {
        static constexpr uint16_t opcode = 0x8ab7;
        static constexpr auto expected = "SUBN Vx, Vy";
        STATIC_REQUIRE(chip8::opcode_to_assembler(opcode) == expected);
    }

    SECTION("invalid opcode") {
        static constexpr uint16_t opcode = 0xE000;
        STATIC_REQUIRE(chip8::opcode_to_assembler(opcode) == "Invalid opcode");
    }
}