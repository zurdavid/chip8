#include <catch2/catch.hpp>

#include "chip8/OpcodeToString.h"
#include "chip8/Chip8.h"

namespace chip8_tests {
    using namespace std::literals::string_view_literals;

    template<std::size_t N>
    consteval std::array<uint8_t, 2UL * N> to_bit8_program(const std::array<uint16_t, N> &program) {
        std::array<uint8_t, 2UL * N> result{0};
        for (std::size_t i = 0; i < N; i++) {
            result.at(i * 2) = (program[i] >> 8) & 0xFFU;
            result.at(i * 2 + 1) = program[i] & 0xFFU;
        }
        return result;
    }

    void load_and_run(chip8::Chip8 &chip8, auto program) {
        chip8.load_rom(program);
        for (uint32_t i = 0; i < program.size() / 2; i++) { chip8.exec_op_cycle(); }
    }

    TEST_CASE("translate to assembler")
    {
        auto opcode = uint16_t{0x8235};
        auto assembler = chip8::opcode_to_assembler(opcode);
        REQUIRE(assembler == "SUB Vx, Vy"sv);
    }


    TEST_CASE("op_ld_vx_nn load byte to register Vx")
    {
        chip8::Chip8 chip8;
        load_and_run(chip8,
                     to_bit8_program<2>({
                                                0x630C, // ld vx nn
                                                0x6719 // ld vx nn
                                        }));

        REQUIRE(chip8.get_registers()[3] == 0x0C);
        REQUIRE(chip8.get_registers()[7] == 0x19);
    }


    TEST_CASE("op_add_vx_vy")
    {
        chip8::Chip8 chip8;

        SECTION("without overflow")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x630C, // ld vx nn
                                                    0x6719, // ld vx nn
                                                    0x8374 // add vx vy
                                            }));
            REQUIRE(chip8.get_registers()[3] == 37);
            // carry flag is zero
            REQUIRE(chip8.get_registers()[0xF] == 0);
        }

        SECTION("with overflow")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x63FF, // ld vx nn
                                                    0x6703, // ld vx nn
                                                    0x8374 // add vx vy
                                            }));
            REQUIRE(chip8.get_registers()[3] == 2);
            // carry flag is set
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("add 1 to 0xFF - overflow")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x63FF, // ld vx nn
                                                    0x6701, // ld vx nk
                                                    0x8374 // add vx vk
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0);
            // carry flag is set
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("add 0 to 0xFF - no overflow")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x63FF, // ld vx nn
                                                    0x6700, // ld vx nn
                                                    0x8374 // add vx vy
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0xFF);
            // carry flag is set
            REQUIRE(chip8.get_registers()[0xF] == 0);
        }
    }

    TEST_CASE("op_sub_vx_vy_minus_vx - subtract vx from vy, store in vx")
    {
        chip8::Chip8 chip8;

        SECTION("no borrow: VF == 1")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x6375, // ld vx nn 117
                                                    0x67d2, // ld vx nn 210
                                                    0x8377 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 93);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("with borrow: VF == 0")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x6307, // ld vx nn
                                                    0x6705, // ld vx nn
                                                    0x8377 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 254);
            REQUIRE(chip8.get_registers()[0xF] == 0);
        }

        SECTION("subtract zero - no borrow: VF == 1")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x6300, // ld vx nn
                                                    0x6733, // ld vx nn
                                                    0x8377 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0x33);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("subtract from itself - no borrow: VF == 1")
        {

            load_and_run(chip8,
                         to_bit8_program<2>({
                                                    0x6342, // ld vx nn
                                                    0x8337 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }
    }

    TEST_CASE("op_sub_vx_vy - 0x8xy5 - subtract vy from vx, store in vx")
    {
        chip8::Chip8 chip8;

        SECTION("no borrow: VF == 1")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x63d2, // ld vx nn 210
                                                    0x6775, // ld vx nn 117
                                                    0x8375 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 93);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("with borrow: VF == 0")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x6305, // ld vx nn
                                                    0x6707, // ld vx nn
                                                    0x8375 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 254);
            REQUIRE(chip8.get_registers()[0xF] == 0);
        }

        SECTION("subtract zero - no borrow: VF == 1")
        {
            load_and_run(chip8,
                         to_bit8_program<3>({
                                                    0x6333, // ld vx nn
                                                    0x6700, // ld vx nn
                                                    0x8375 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0x33);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }

        SECTION("subtract from itself - no borrow: VF == 1")
        {

            load_and_run(chip8,
                         to_bit8_program<2>({
                                                    0x6342, // ld vx nn
                                                    0x8335 // sub vy vx
                                            }));
            REQUIRE(chip8.get_registers()[3] == 0);
            REQUIRE(chip8.get_registers()[0xF] == 1);
        }
    }

    TEST_CASE("call and return from subroutine")
    {
        chip8::Chip8 chip8;

        SECTION("call subroutine - 0x2nnn")
        {
            load_and_run(chip8, to_bit8_program<1>({
                                                           0x2276, // call subroutine at 0x111
                                                   }));
            REQUIRE(chip8.get_pc() == 0x276);
        }

        SECTION("jump to subroutine and return from subroutine - 0x00EE")
        {
            chip8.load_rom(to_bit8_program<7>({
                                                      0x220A, // call subroutine
                                                      0x0002, // invalid
                                                      0x0004, // invalid
                                                      0x0006, // invalid
                                                      0x0008, // invalid
                                                      0x00EE, // return from subroutine
                                              }));
            chip8.exec_op_cycle();
            chip8.exec_op_cycle();

            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 2);
        }
    }

    TEST_CASE("op_goto - 0x1nnn")
    {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<1>({
                                                       0x1234, // call subroutine at 0x111
                                               }));
        REQUIRE(chip8.get_pc() == 0x234);
    }

    TEST_CASE("op_skip_ifeq_vx_nn - 0x3xnn") {
        chip8::Chip8 chip8;
        SECTION("equal - skip") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6571, // ld vx nn
                                                           0x3571, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 6); // two executed 1 skipped
        }

        SECTION("unequal - don't skip") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6571, // ld vx nn
                                                           0x3514,
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 4); // two executed
        }
    }


    TEST_CASE("op_skip_ifneq_vx_nn - 0x4xnn") {
        chip8::Chip8 chip8;

        SECTION("equal - skip") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6571, // ld vx nn
                                                           0x4571, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 4); // two executed
        }

        SECTION("unequal - don't skip") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6571, // ld vx nn
                                                           0x4514,
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 6); // two executed + 1 skipped
        }
    }


    TEST_CASE("op_skip_ifeq_xy - 0x5xy0") {
        chip8::Chip8 chip8;

        SECTION("equal - skip") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6171, // ld vx nn
                                                           0x6771, // ld vx nn
                                                           0x5170, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 8);
        }

        SECTION("unequal - don't skip") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6171, // ld vx nn
                                                           0x6711, // ld vx nn
                                                           0x5170, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 6);
        }
    }


    TEST_CASE("op_skip_ifneq_xy - 0x9xy0") {
        chip8::Chip8 chip8;

        SECTION("equal - don't skip") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6171, // ld vx nn
                                                           0x6771, // ld vx nn
                                                           0x9170, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 6);
        }

        SECTION("unequal - skip") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6171, // ld vx nn
                                                           0x6711, // ld vx nn
                                                           0x9170, // skip
                                                   }));
            REQUIRE(chip8.get_pc() == chip8.pc_start_address + 8);
        }
    }


    TEST_CASE("op_add_vx_nn - 0x7xnn") {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<2>({
            0x6777, // ld vx nn
            0x7742,
        }));
        REQUIRE(chip8.get_registers()[7] == 0x77 + 0x42);
    }


    TEST_CASE("op_ld_vx_vy - 0x8xy0") {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<2>({
                                                       0x6744, // ld vx nn
                                                       0x8470, // assign x y
                                               }));
        REQUIRE(chip8.get_registers()[4] == 0x44);
    }

    TEST_CASE("op_or_vx_vy - 0x8xxy1") {
        chip8::Chip8 chip8;

        SECTION("or different bits set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640C, // ld vx nn
                                                           0x6790, // ld vx nn
                                                           0x8471, // or x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x9C);
        }
        SECTION("or some bits set the same") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x646F, // ld vx nn
                                                           0x6757, // ld vx nn
                                                           0x8471, // or x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x7F);
        }
    }

    TEST_CASE("op_and_vx_vy - 0x8xy2") {
        chip8::Chip8 chip8;

        SECTION("and different bits set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640C, // ld vx nn
                                                           0x6790, // ld vx nn
                                                           0x8472 // and x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x00);
        }
        SECTION("and some bits set the same") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x646F, // ld vx nn
                                                           0x6757, // ld vx nn
                                                           0x8472 // and x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x47);
        }
    }

    TEST_CASE("op_xor_vx_vy - 0x8xy3") {
        chip8::Chip8 chip8;

        SECTION("xor different bits set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640C, // ld vx nn
                                                           0x6790, // ld vx nn
                                                           0x8473 // xor x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x9C);
        }
        SECTION("xor some bits set the same") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x646F, // ld vx nn
                                                           0x6757, // ld vx nn
                                                           0x8473 // xor x y
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x38);
        }
    }

    TEST_CASE("op_rshift - 0x8xy6") {
        chip8::Chip8 chip8;
        chip8.set_shift_implementation(true);

        SECTION("right shift - Vy to Vx") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x670F, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x07);
            REQUIRE(chip8.get_registers()[7] == 0x0F);
        }
        SECTION("right shift - least significant bit set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x670F, // ld vx nn
                                                           0x6F00, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x1);
        }
        SECTION("right shift - least significant bit not set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6704, // ld vx nn
                                                           0x6F01, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x0);
        }
        SECTION("right shift - Vx to Vx, Vy unchanged") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640E, // ld vx nn
                                                           0x67FF, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x07);
            REQUIRE(chip8.get_registers()[7] == 0xFF);
        }
        SECTION("right shift - Vx to Vy - least significant bit set") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640F, // ld vx nn
                                                           0x6F00, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x01);
        }
        SECTION("right shift - Vx to Vy - least significant bit not set") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640E, // ld vx nn
                                                           0x6F01, // ld vx nn
                                                           0x8476  // rshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x00);
        }
    }

    TEST_CASE("op_lshift - 0x8xyE") {
        chip8::Chip8 chip8;
        chip8.set_shift_implementation(true);

        SECTION("left shift - Vy to Vx") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x670F, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x1E);
            REQUIRE(chip8.get_registers()[7] == 0x0F);
        }
        SECTION("left shift - least significant bit set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x67F8, // ld vx nn
                                                           0x6F00, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x1);
        }
        SECTION("left shift - least significant bit not set") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6704, // ld vx nn
                                                           0x6F01, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x0);
        }
        SECTION("left shift - Vx to Vx, Vy unchanged") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640E, // ld vx nn
                                                           0x67FF, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[4] == 0x1C);
            REQUIRE(chip8.get_registers()[7] == 0xFF);
        }
        SECTION("left shift - Vx to Vx - least significant bit set") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x64F0, // ld vx nn
                                                           0x6F00, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x1);
        }
        SECTION("left shift - Vx to Vx - least significant bit not set") {
            chip8.set_shift_implementation(false);
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x640E, // ld vx nn
                                                           0x6F01, // ld vx nn
                                                           0x847E  // lshift vx vy
                                                   }));
            REQUIRE(chip8.get_registers()[0xF] == 0x0);
        }
    }


    TEST_CASE("op_ld_i_nnn - 0xAnnn") {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<1>({
                                                       0xA123, // set i nnn
                                               }));
        REQUIRE(chip8.get_i() == 0x123);
    }


    TEST_CASE("op_goto_i_plus_v0 - 0xBnnn") {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<2>({
                                                       0x6008, // ld vx nn
                                                       0xB123, // goto i plus v0
                                               }));
        REQUIRE(chip8.get_pc() == 0x12B);
    }

    TEST_CASE("op_add_to_I 0xF01E") {
        chip8::Chip8 chip8;
        load_and_run(chip8, to_bit8_program<3>({
                                                       0x6708, // ld vx nn
                                                       0xA234, // ld I nnn
                                                       0xF71E, // goto i plus v0
                                               }));
        REQUIRE(chip8.get_i() == 0x23C);
    }

    TEST_CASE("op_vx_to_BCD - 0xFx33") {
        chip8::Chip8 chip8;
        SECTION("255 (0xFF)") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0xA1AB, // ld I nnn
                                                           0x67FF, // ld vx nn
                                                           0xF733, // vx to bcd
                                                   }));
            const auto I = chip8.get_i();
            REQUIRE(I == 0x1AB);
            REQUIRE(chip8.get_memory()[I] == 2);
            REQUIRE(chip8.get_memory()[I+1] == 5);
            REQUIRE(chip8.get_memory()[I+2] == 5);
        }
        SECTION("79 (0x4f)") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0xA1AB, // ld I nnn
                                                           0x674F, // ld vx nn
                                                           0xF733, // vx to bcd
                                                   }));
            const auto I = chip8.get_i();
            REQUIRE(I == 0x1AB);
            REQUIRE(chip8.get_memory()[I] == 0);
            REQUIRE(chip8.get_memory()[I+1] == 7);
            REQUIRE(chip8.get_memory()[I+2] == 9);
        }
        SECTION("0") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0xA1AB, // ld I nnn
                                                           0x6700, // ld vx nn
                                                           0xF733, // vx to bcd
                                                   }));
            const auto I = chip8.get_i();
            REQUIRE(I == 0x1AB);
            REQUIRE(chip8.get_memory()[I] == 0);
            REQUIRE(chip8.get_memory()[I+1] == 0);
            REQUIRE(chip8.get_memory()[I+2] == 0);
        }
    }

    TEST_CASE("op_set_I_to_digit_sprite_address - 0xFx29") {
        chip8::Chip8 chip8;
        static constexpr int sprite_height = 5;

        SECTION("zero") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6A00, // ld vx nn
                                                           0xFA29, // set I to digit
                                                   }));
            REQUIRE(chip8.get_i() == 0);
        }

        SECTION("below 10") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6A04, // ld vx nn
                                                           0xFA29, // set I to digit
                                                   }));
            REQUIRE(chip8.get_i() == 4 * sprite_height);
        }

        SECTION("above 9") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6A0A, // ld vx nn
                                                           0xFA29, // set I to digit
                                                   }));
            REQUIRE(chip8.get_i() == 10 * sprite_height);
        }
        SECTION("only least significant hex digit counts") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6A7A, // ld vx nn
                                                           0xFA29, // set I to digit
                                                   }));
            REQUIRE(chip8.get_i() == 10 * sprite_height);
        }
    }

    TEST_CASE("timer operations") {
        chip8::Chip8 chip8;

        SECTION("op_ld_delay_timer_vx - 0xFx15") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6814, // ld vx nn
                                                           0xF815, // ld delay_timer vx
                                                   }));
            REQUIRE(chip8.get_delay_timer() == 0x14);
        }

        SECTION("op_ld_sound_timer_vx - 0xFx18") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6321, // ld vx nn
                                                           0xF318, // ld sound_timer vx
                                                   }));
            REQUIRE(chip8.get_sound_timer() == 0x21);
        }

        SECTION("op_ld_vx_delay_timer - 0xFx07") {
            load_and_run(chip8, to_bit8_program<3>({
                                                           0x6333, // ld vx nn
                                                           0xF315, // ld delay_timer vx
                                                           0xF507, // ld vx delay_timer
                                                   }));
            REQUIRE(chip8.get_registers()[5] == 0x33);
        }
    }

    TEST_CASE("op_skip_if_key_vx_pressed - 0xEx9E") {
        chip8::Chip8 chip8;
        const auto key = 0x3;
        SECTION("key pressed - skip") {
            chip8.keys[key] = true;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6603, // ld vx nn
                                                           0xE69E, // skip if key vx pressed
                                                   }));
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 6);
        }
        SECTION("key not pressed - don't skip") {
            chip8.keys[key] = false;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6603, // ld vx nn
                                                           0xE39E, // skip if key vx pressed
                                                   }));
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 4);
        }
    }


    TEST_CASE("op_skip_if_key_vx_not_pressed - 0xExA1") {
        chip8::Chip8 chip8;
        const auto key = 0xA;
        SECTION("key pressed - don't skip") {
            chip8.keys[key] = true;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x620A, // ld vx nn
                                                           0xE2A1, // skip if key vx not pressed
                                                   }));
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 4);
        }
        SECTION("key not pressed - skip") {
            chip8.keys[key] = false;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x620A, // ld vx nn
                                                           0xE2A1, // skip if key vx not pressed
                                                   }));
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 6);
        }
    }


    TEST_CASE("op_get_key_pressed - 0xFx0A") {
        chip8::Chip8 chip8;
        std::fill(chip8.keys.begin(), chip8.keys.begin(), false);

        SECTION("no key pressed - block") {
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6DFF, // ld vx nn
                                                           0xFD0A, // get key pressed
                                                   }));
            REQUIRE(chip8.get_registers()[0xD] == 0xFF);
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 2);
        }

        SECTION("key 7 pressed") {
            chip8.keys[7] = true;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6DFF, // ld vx nn
                                                           0xFD0A, // get key pressed
                                                   }));
            REQUIRE(chip8.get_registers()[0xD] == 0x07);
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 4);
        }

        SECTION("keys A and E pressed") {
            chip8.keys[0xE] = true;
            chip8.keys[0xA] = true;
            load_and_run(chip8, to_bit8_program<2>({
                                                           0x6DFF, // ld vx nn
                                                           0xFD0A, // get key pressed
                                                   }));
            REQUIRE(chip8.get_registers()[0xD] == 0x0A);
            REQUIRE(chip8.get_pc() == chip8::Chip8::pc_start_address + 4);
        }
    }

    TEST_CASE("op_regdump - 0xFx55")
    {
        chip8::Chip8 chip8;
        // dump registers 0 to 0xB == 11 in memory
        // starting at address I
        load_and_run(chip8, to_bit8_program<9>({

                                                       0x6000, // ld vx nn
                                                       0x6303, // ld vx nn
                                                       0x64A4, // ld vx nn
                                                       0x6909, // ld vx nn
                                                       0x6B0B, // ld vx nn
                                                       0x6CFF, // ld vx nn
                                                       0x6FFF, // ld vx nn
                                                       0xA300, // ld I nnn
                                                       0xFB55, // regdump
                                               }));
        const auto &mem = chip8.get_memory();
        const auto &V = chip8.get_registers();
        for (uint32_t i = 0; i <= 0xB; i++) {
            REQUIRE(mem[0x300 + i] == V[i]);
        }
        REQUIRE(mem[0x30C] != V[12]);
        REQUIRE(mem[0x30F] != V[15]);
    }

    TEST_CASE("op_regload - 0xFx65")
    {
        chip8::Chip8 chip8;

        // load memory into registers 0 to 0xB (11)
        // starting from address I
        chip8.load_rom(to_bit8_program<10>({
            0xA204, // ld I nnn
            0xFB55, // regload
            0x6000, // memory to be loaded
            0x0123,
            0xFABA,
            0xABCD,
            0x1234,
            0x5678,
            0x1234,
            0xABCD
        }));
        chip8.exec_op_cycle();
        chip8.exec_op_cycle();
        const auto &mem = chip8.get_memory();
        const auto &V = chip8.get_registers();
        const auto start = 0x204;
        for (uint32_t i = 0; i <= 0xB; i++) {
            REQUIRE(mem[start + i] == V[i]);
        }
        for (uint32_t i = 0xC; i <= 0xF; i++) {
            REQUIRE(mem[start + i] != V[i]);
        }
    }




} // namespace chip8_tests
