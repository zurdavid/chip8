#include "chip8/Chip8.h"
#include <fstream>
#include <iterator>
#include <random>
#include <algorithm>
#include <bitset>
#include <functional>
#include <gsl/narrow>
#include "utilities/Map.h"
#include <ranges>
#include <spdlog/spdlog.h>


namespace chip8 {
    namespace ranges = std::ranges;

    static constexpr auto sprite_size = int{5};
    static constexpr auto bytes_in_screen = 8 * Chip8::screen_height;
    static constexpr auto F = int{0xF};
    static constexpr auto xFF = 0xFFU;
    static constexpr auto program_start = uint16_t{512};

    // sprites
    static constexpr std::array<uint8_t, 80> fontset = {{
            0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
            0x20, 0x60, 0x20, 0x20, 0x70,  // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
            0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
            0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
            0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
            0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
            0xF0, 0x80, 0xF0, 0x80, 0x80   // F
    }};

    std::mt19937 getRandomGenerator() {
        std::random_device rd;
        std::mt19937 gen(rd());
        return gen;
    }

    static auto random_generator = getRandomGenerator(); // NOLINT if it throws, app crashes

    Chip8::Chip8() {
        ranges::copy(fontset, memory.begin());
        op_clear_screen(0);
    }


    void Chip8::load_rom(const std::string &filename) {
        std::ifstream rom_file(filename);
        if (rom_file) {
            rom_file >> std::noskipws;
            std::vector<uint8_t> rom((std::istream_iterator<uint8_t>(rom_file)),
                                     (std::istream_iterator<uint8_t>()));
            ranges::copy(rom, memory.begin() + program_start);
            program_size = rom.size();
            reset();
            draw_flag = true;
        } else {
            // TODO give user error message
            spdlog::error("Could not open file: {}", filename);
        }
    }


    void Chip8::exec_op_cycle() {
        const auto opcode = gsl::narrow_cast<uint16_t>((memory[PC] << 8) | memory[PC + 1]); // NOLINT (cppcoreguidelines-pro-bounds-constant-array-index)
        incPC();
        const auto op = fetch_op(opcode);
        std::invoke(op, this, opcode);
        call_stack.push_front(opcode);
        if (call_stack.size() > call_stack_size) { call_stack.pop_back(); }
        tick_count++;
    }


    namespace {
    // get 4 bit starting with bit-position startbit
    [[nodiscard]] constexpr uint16_t get4Bit(const uint16_t opcode, const uint16_t start_bit) {
        constexpr auto mask = uint16_t{0xFU};
        return (opcode >> start_bit) & mask;
    }


    // returns X in 0x_X__
    [[nodiscard]] constexpr uint16_t X(uint16_t opcode) {
        return get4Bit(opcode, 8);
    }


    // returns X in 0x__Y_
    [[nodiscard]] constexpr uint16_t Y(uint16_t opcode) {
        return get4Bit(opcode, 4);
    }


    // returns X in 0x___N
    [[nodiscard]] constexpr uint16_t n(uint16_t opcode) {
        return get4Bit(opcode, 0);
    }

    // returns X in 0x__NN
    [[nodiscard]] constexpr uint8_t nn(uint16_t opcode) {
        return gsl::narrow_cast<uint8_t>(opcode & 0xFFU);
    }


    // returns X in 0x_NNN
    [[nodiscard]] constexpr uint16_t nnn(uint16_t opcode) {
        constexpr auto mask = uint16_t{0xFFF};
        return opcode & mask;
    }
}

    // clear screen
    void Chip8::op_clear_screen(uint16_t) { // NOLINT opcode is not needed
        ranges::fill(display_buffer, 0);
        draw_flag = true;
    }


    // return from subroutine
    void Chip8::op_return_from_subroutine(uint16_t) { // NOLINT opcode is not needed
        PC = stack.top();
        stack.pop();
    }


    // Jumps to address NNN.
    void Chip8::op_goto(uint16_t opcode) {
        PC = nnn(opcode);
    }


    // jumps to subroutinge
    void Chip8::op_call_subroutine(uint16_t opcode) {
        stack.push(PC);
        PC = nnn(opcode);
    }


    // Skips the next instruction if VX equals NN.
    void Chip8::op_skip_ifeq(const uint16_t opcode) {
        if (V[X(opcode)] == nn(opcode)) {
            incPC();
        }
    }


    // Skips the next instruction if VX does not equal NN.
    void Chip8::op_skip_ifneq(uint16_t opcode) {
        if (V[X(opcode)] != nn(opcode)) {
            incPC();
        }
    }


    // Skips the next instruction if VX equals VY.
    void Chip8::op_skip_ifeq_xy(uint16_t opcode) {
        if (V[X(opcode)] == V[Y(opcode)]) {
            incPC();
        }
    }


    // 0x6xnn - Sets Vx to nn
    void Chip8::ld_vx_nn(uint16_t opcode) {
        V[X(opcode)] = nn(opcode);
    }


    // Adds NN to VX. (Carry flag is not changed)
    void Chip8::op_add_nn(uint16_t opcode) {
        V[X(opcode)] += nn(opcode);
    }


    // Sets VX to the value of VY.
    void Chip8::op_assign(uint16_t opcode) {
        V[X(opcode)] = V[Y(opcode)];
    }


    // Sets VX to VX or VY. (Bitwise OR operation)
    void Chip8::op_or(uint16_t opcode) {
        V[X(opcode)] |= V[Y(opcode)];
    }


    // Sets VX to VX and VY. (Bitwise AND operation)
    void Chip8::op_and(uint16_t opcode) {
        V[X(opcode)] &= V[Y(opcode)];
    }


    // Sets VX to VX xor VY. (Bitwise XOR operation)
    void Chip8::op_xor(uint16_t opcode) {
        V[X(opcode)] ^= V[Y(opcode)];
    }


    // 8XY4 - Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
    void Chip8::op_add(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = Y(opcode);
        const auto vx = V[x];
        V[x] += V[y];
        V[F] = vx > V[x]; // NOLINT (readability-implicit-bool-conversion)
    }


    // 8XY5 - VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void Chip8::op_sub(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = Y(opcode);
        const auto vx = V[x];
        V[x] -= V[y];
        V[F] = vx >= V[x]; // NOLINT (readability-implicit-bool-conversion)
    }


    // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void Chip8::op_sub_rev(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = Y(opcode);
        const auto vy = V[y];
        V[x] = V[y] - V[x];
        V[F] = vy >= V[x]; // NOLINT (readability-implicit-bool-conversion)
    }


    // Store the value of register VY shifted right one bit in register VX¹
    // Set register VF to the least significant bit prior to the shift
    // VY is unchanged
    // Changed according to: https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
    void Chip8::op_rshift(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = shift_implementation_vy ? Y(opcode) : X(opcode);
        V[F] = V[y] & 0b1U;
        V[x] = V[y] >> 1U;
    }


    // Store the value of register VY shifted left one bit in register VX
    // Set register VF to the most significant bit prior to the shift
    // VY is unchanged
    // Changed according to: https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
    void Chip8::op_lshift(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = shift_implementation_vy ? Y(opcode) : X(opcode);
        V[F] = V[y] >> 7;
        V[x] = gsl::narrow_cast<uint8_t>((V[y] << 1U) & 0xFF);
    }


    // Skips the next instruction if VX does not equal VY.
    void Chip8::op_skip_ifneq_xy(uint16_t opcode) {
        if (V[X(opcode)] != V[Y(opcode)]) {
            incPC();
        }
    }


    // Sets I to the address NNN.
    void Chip8::op_set_i(uint16_t opcode) {
        I = nnn(opcode);
    }


    // Jumps to the address NNN plus V0.
    void Chip8::op_goto_plus_reg(uint16_t opcode) {
        PC = nnn(opcode) + V[0];
    }


    // Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
    void Chip8::op_and_rand(uint16_t opcode) {
        std::uniform_int_distribution<int> distrib(0, xFF);
        const auto random_number = gsl::narrow_cast<uint8_t>(distrib(random_generator) & 0xFF);
        const auto val = nn(opcode);
        V[X(opcode)] = random_number & val;
    }


    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N+1 pixels.
    // Each row of 8 pixels is read as bit-coded starting from memory location I;
    // I value does not change after the execution of this instruction.
    // As described above, VF is set to 1 if any screen pixels are flipped
    // from set to unset when the sprite is drawn, and to 0 if that does not happen
    // Sprites that don't fit on the screen wrap around the screen (show on the other end).
    void Chip8::op_draw(uint16_t opcode) {
        const auto vx = V[X(opcode)] % screen_width;
        const auto vy = V[Y(opcode)] % screen_height;
        const auto N = static_cast<int>(n(opcode));

        const auto byte = vx / 8;
        const auto offset = vx % 8;

        bool flipped = false;
        // what is happening here
        for (int line = 0; line < N; line++) {
            const auto index = static_cast<uint32_t>(I + line);
            const auto sprite_line = memory[index];

            // sprites that don't fit on the screen wrap around the screen (show on the other end).
            const auto left_byte_idx = gsl::narrow_cast<uint8_t>(byte + ((vy + line) % 32) * 8);
            const auto old_left = display_buffer[left_byte_idx];
            display_buffer[left_byte_idx] = old_left ^ (sprite_line >> offset);
            if (old_left & (~display_buffer[left_byte_idx])) { // NOLINT (readability-implicit-bool-conversion)
                flipped = true;
            }

            // sprites that don't fit on the screen wrap around the screen (show on the other end).
            const auto right_byte_idx = gsl::narrow_cast<uint8_t>((byte + 1) % 8 + ((vy + line) % 32) * 8);
            const auto old_right = display_buffer[right_byte_idx];
            display_buffer[right_byte_idx] = (old_right ^ ((sprite_line << (8 - offset)) & 0xFF));
            if (old_right & (~display_buffer[right_byte_idx])) { // NOLINT (readability-implicit-bool-conversion)
                flipped = true;
            }
        }
        V[F] = static_cast<uint8_t>(flipped);
        draw_flag = true;
    }


    // Skips the next instruction if the key stored in VX is pressed.
    void Chip8::op_keyop1(uint16_t opcode) {
        const auto vx = get4Bit(V[X(opcode)], 0);
        if (keys[vx]) {
            incPC();
        }
    }


    // Skips the next instruction if the key stored in VX is not pressed.
    void Chip8::op_keyop2(uint16_t opcode) {
        const auto vx = get4Bit(V[X(opcode)], 0);
        if (!keys[vx]) {
            incPC();
        }
    }


    // Sets VX to the value of the delay timer.
    void Chip8::op_to_delay_timer(uint16_t opcode) {
        V[X(opcode)] = delay_timer;
    }


    // A key press is awaited, and then stored in VX.
    // (should be a blocking operation. All instruction halted until next key event)
    // This implementations only simulates a blocking by not increasing the PC
    void Chip8::op_get_key(uint16_t opcode) {
        // check all keys
        for (std::size_t key_idx = 0; auto key_pressed: keys) {
            if (key_pressed) {
                V[X(opcode)] = static_cast<uint8_t >(key_idx);
                keys[key_idx] = false;
                return;
            }
            key_idx++;
        }
        // if no key was pressed, decrease the instruction counter, so
        // the instruction will be called again (simulates blocking).
        PC -= 2;
    }


    // Sets the delay timer to VX.
    void Chip8::op_set_delay_timer(uint16_t opcode) {
        delay_timer = V[X(opcode)];
    }


    // Sets the sound timer to VX.
    void Chip8::op_set_sound_timer(uint16_t opcode) {
        sound_timer = V[X(opcode)];
    }


    // Adds VX to I. VF is not affected.[
    void Chip8::op_add_to_I(uint16_t opcode) {
        I += V[X(opcode)];
    }


    // 0xFX29 - Set I = location of sprite for digit Vx.
    void Chip8::op_set_I_to_sprite_address(uint16_t opcode) {
        I = sprite_size * get4Bit(V[X(opcode)], 0);
    }


    // Stores the binary-coded decimal representation of VX,
    // with the most significant of three digits at the address in I,
    // the middle digit at I plus 1, and the least significant digit at I plus 2.
    void Chip8::op_set_BCD(uint16_t opcode) {
        const auto vx = V[X(opcode)];
        memory[I] = vx / 100;
        memory[I + 1] = (vx % 100) / 10;
        memory[I + 2] = vx % 10;
    }


    // FX55 - LD [I], Vx
    // Stores V0 to VX (including VX) in memory starting at address I.
    // I is set to I + X + 1 after operation --> see https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
    void Chip8::op_regdump(uint16_t opcode) {
        const uint16_t x = X(opcode) + 1;
        ranges::copy_n(V.begin(), x, memory.begin() + I);
        I += x;
    }


    // FX65 - LD Vx, [I]
    // Fills V0 to VX (including VX) with values from memory starting at address I.
    // I is set to I + X + 1 after operation --> see https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
    void Chip8::op_regload(uint16_t opcode) {
        const uint16_t x = X(opcode) + 1;
        std::copy_n(memory.begin() + I, x, V.begin());
        I += x;
    }


    Chip8::MFP Chip8::fetch_op(uint16_t opcode) {
        // masks is used to hide the variable parts of opcodes, so the opcode can be
        // looked up in operations
        static constexpr auto masks = std::array<uint16_t, 16>{
                0xFFFF, 0xF000, 0xF000, 0xF000,
                0xF000, 0xF000, 0xF000, 0xF000,
                0xF00F, 0xF000, 0xF000, 0xF000,
                0xF000, 0xF000, 0xF0FF, 0xF0FF
        };
        static constexpr auto map = Map<uint16_t, MFP, num_opcodes>{{operations}}; // NOLINT

        const auto idx = get4Bit(opcode, 12);
        const auto mask = masks[idx];
        const auto op = map.at(opcode & mask);
        return op;
    }


    std::array<uint8_t, Chip8::screen_size> Chip8::get_screen() const {
        std::array<uint8_t, Chip8::screen_size> screen{0};

        // for_each byte of the display_buffer
        // set 8 fields of the screen array
        std::for_each(
                display_buffer.rbegin(), display_buffer.rend(),

                [&screen, idx = std::size_t{0}](uint8_t byte) mutable {
                    std::bitset<8> bitIsSet{byte};
                    for (std::size_t i = 0; i < 8; i++) {
                        screen[idx] = bitIsSet[i] ? 0xFF : 0x0;
                        idx++;
                    }
                });
        return screen;
    }


    void Chip8::incPC() {
        static constexpr uint16_t inc = 2;
        PC += inc;
    }


    void Chip8::signal() {
        delay_timer = gsl::narrow_cast<uint8_t>(std::max(delay_timer - 1, 0));
        sound_timer = gsl::narrow_cast<uint8_t >(std::max(sound_timer - 1, 0));
    }


    void Chip8::reset() {
        state = State::Reset;

        PC = pc_start_address;
        I = 0;

        static constexpr std::array<uint8_t, bytes_in_screen> start_screen{
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0xef, 0x7b, 0xde, 0x3d, 0xef, 0x7b, 0xc0,
                0x1, 0x29, 0x42, 0x10, 0x21, 0x29, 0x42, 0x0, 0x1, 0xef, 0x73, 0xde, 0x3d, 0xef, 0x43, 0x80,
                0x1, 0xa, 0x40, 0x42, 0x5, 0x9, 0x42, 0x0, 0x1, 0x9, 0x7b, 0xde, 0x3d, 0x9, 0x7b, 0xc0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x7, 0xde, 0x1e, 0xfb, 0xde, 0xf8, 0x0, 0x0, 0x1, 0x12, 0x10, 0x22, 0x52, 0x20, 0x0,
                0x0, 0x1, 0x12, 0x1e, 0x23, 0xde, 0x20, 0x0, 0x0, 0x1, 0x12, 0x2, 0x22, 0x54, 0x20, 0x0,
                0x0, 0x1, 0x1e, 0x1e, 0x22, 0x52, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        };
        ranges::copy(start_screen, display_buffer.begin());

        // clear registers
        ranges::fill(V, 0);
        // clear stack
        stack = {};
        delay_timer = 0;
        sound_timer = 0;

        tick_count = 0;
        call_stack.clear();
    }


    void Chip8::toggle_pause() {
        switch (state) {
            case State::Running:
                state = State::Paused;
                break;
            case State::Paused:
                state = State::Running;
                break;
            case State::Reset:
                op_clear_screen(0);
                state = State::Running;
                break;
            case State::Empty:
                break;
        }
    }


    void Chip8::set_shift_implementation(bool shift_vy) {
        shift_implementation_vy = shift_vy;
    }


    void Chip8::tick() {
        if (state == State::Running) {
            signal();
            try {
                for (auto i = 0; i < cycles_per_frame; i++) {
                    exec_op_cycle();
                }
            } catch (std::range_error &e) {
                spdlog::error("Invalid opcode!\n {}", e.what());
                error();
            }
        }
    }

    void Chip8::error() {
        state = State::Empty;
        static constexpr std::array<uint8_t, bytes_in_screen> error_screen{
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x3f, 0xc3, 0xf8, 0x1f, 0xc0, 0x7f, 0xf, 0xe0,
                0x3f, 0xc3, 0xfc, 0x1f, 0xe0, 0xff, 0x8f, 0xf0, 0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18,
                0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18, 0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18,
                0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18, 0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18,
                0x3f, 0xc3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18, 0x3f, 0xc3, 0xfc, 0x1f, 0xe0, 0xc1, 0x8f, 0xf0,
                0x30, 0x3, 0xf8, 0x1f, 0xc0, 0xc1, 0x8f, 0xe0, 0x30, 0x3, 0x18, 0x18, 0xc0, 0xc1, 0x8c, 0x60,
                0x30, 0x3, 0xc, 0x18, 0x60, 0xc1, 0x8c, 0x30, 0x30, 0x3, 0xc, 0x18, 0x60, 0xc1, 0x8c, 0x30,
                0x30, 0x3, 0x6, 0x18, 0x30, 0xc1, 0x8c, 0x18, 0x3f, 0xc3, 0x6, 0x18, 0x30, 0xff, 0x8c, 0x18,
                0x3f, 0xc3, 0x6, 0x18, 0x30, 0x7f, 0xc, 0x18, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        };
        ranges::copy(error_screen, display_buffer.begin());
        draw_flag = true;
        // display error
    }


} // namespace chip8
