#include "chip8/Chip8.h"
#include <fstream>
#include <iterator>
#include <random>
#include <bitset>
#include <ranges>
#include <functional>
#include "utilities/Map.h"

#include <spdlog/spdlog.h>

// /TODO maybe delete
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"

namespace chip8 {

    static constexpr auto SIZEOFSPRITE = int{5};
    static constexpr auto F = int{0xF};
    static constexpr auto PROGRAM_START = uint16_t{512};

    static constexpr auto masks = std::array<uint16_t, 16>{
      0xFFFF, 0xF000, 0xF000, 0xF000,
      0xF000, 0xF000, 0xF000, 0xF000,
      0xF00F, 0xF000, 0xF000, 0xF000,
      0xF000, 0xF000, 0xF0FF, 0xF0FF
    };

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
    static auto random_generator = getRandomGenerator();


    Chip8::Chip8() {
        std::copy(fontset.begin(), fontset.end(), memory.begin());
        op_clear_screen(0);
    }


    void Chip8::load_rom(const std::string &filename) {
        std::ifstream rom_file(filename);
        if (rom_file) {
          rom_file >> std::noskipws;
          std::vector<uint8_t> v((std::istream_iterator<uint8_t>(rom_file)),
                                 (std::istream_iterator<uint8_t>()));
          std::copy(v.begin(), v.end(), memory.begin() + PROGRAM_START);
          program_size = v.size();
        } else {
          spdlog::error("Could not open file: {}", filename);
        }
        reset();
        is_running = true;
        draw_flag = true;
    }


    void Chip8::exec_op_cycle() {
        const uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
//        spdlog::error("opcode: {0:X}", opcode);
        incPC();
        const auto op = fetch_op(opcode);
        std::invoke(op, this, opcode);
    }


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
    [[nodiscard]] constexpr uint16_t nn(uint16_t opcode) {
        constexpr auto mask = uint16_t{0xFF};
        return opcode & mask;
    }


    // returns X in 0x_NNN
    [[nodiscard]] constexpr uint16_t nnn(uint16_t opcode) {
        constexpr auto mask = uint16_t{0xFFF};
        return opcode & mask;
    }


    // clear screen
    void Chip8::op_clear_screen(uint16_t) {
        std::fill(display_buffer.begin(), display_buffer.end(), 0);
        draw_flag = true;
    }


    // return from subroutine
    void Chip8::op_return_from_subroutine(uint16_t) {
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
        V[F] = vx > V[x];
    }


    // 8XY5 - VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void Chip8::op_sub(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = Y(opcode);
        const auto vx = V[x];
        V[x] -= V[y];
        V[F] = vx > V[x];
    }


    // Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
    void Chip8::op_rshift(uint16_t opcode) {
        const auto x = X(opcode);
        V[F] = V[x] & 0b1;
        V[x] >>= 1;
    }


    // Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void Chip8::op_sub_rev(uint16_t opcode) {
        const auto x = X(opcode);
        const auto y = Y(opcode);
        const auto vy = V[y];
        V[x] = V[y] - V[x];
        V[F] = vy > V[x];
    }


    // Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
    void Chip8::op_lshift(uint16_t opcode) {
        static constexpr auto most_significant_bit_mask = uint16_t{0b10000000};
        V[F] = V[X(opcode)] & most_significant_bit_mask;
        V[X(opcode)] <<= 1U;
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
        std::uniform_int_distribution<uint8_t> distrib(0, 255);
        const auto random_number = distrib(random_generator);
        const auto val = nn(opcode);
        V[X(opcode)] = random_number & val;
    }


    // Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N+1 pixels.
    // Each row of 8 pixels is read as bit-coded starting from memory location I;
    // I value does not change after the execution of this instruction.
    // As described above, VF is set to 1 if any screen pixels are flipped
    // from set to unset when the sprite is drawn, and to 0 if that does not happen
    void Chip8::op_draw(uint16_t opcode) {
        const auto vx = V[X(opcode)];
        const auto vy = V[Y(opcode)];
        const auto N = static_cast<int>(n(opcode));

        const auto byte = vx / 8;
        const auto offset = vx % 8;

        bool flipped = false;
        for (std::weakly_incrementable auto i : std::views::iota(0, N)) {
            const auto coord1 = byte + (vy + i) * 8;
            const auto old = display_buffer[coord1];
            display_buffer[coord1] = old ^ (memory[I + i] >> offset);
            if (old & (~display_buffer[coord1])) {
                flipped = true;
            }

            const auto coord2 = (byte + 1) + (vy + i) * 8;
            const auto old2 = display_buffer[coord2];
            display_buffer[coord2] = old2 ^ (memory[I + i] << (8 - offset));
            if (old2 & (~display_buffer[coord2])) {
                flipped = true;
            }
        }
        V[F] = static_cast<uint8_t>(flipped);
        draw_flag = true;
    }


    // Skips the next instruction if the key stored in VX is pressed.
    void Chip8::op_keyop1(uint16_t opcode) {
        auto vx = V[X(opcode)];
        if (keys[vx]) {
            incPC();
        }
    }


    // Skips the next instruction if the key stored in VX is not pressed.
    void Chip8::op_keyop2(uint16_t opcode) {
        auto vx = V[X(opcode)];
        if (!keys[vx]) {
            incPC();
        }
    }


    // Sets VX to the value of the delay timer.
    void Chip8::op_to_delay_timer(uint16_t opcode) {
        V[X(opcode)] = delay_timer;
    }


    // A key press is awaited, and then stored in VX.
    // (Blocking Operation. All instruction halted until next key event)
    void Chip8::op_get_key(uint16_t opcode) {
        // check all keys
        for (int key_idx = 0; auto key_pressed : keys) {
            if (key_pressed) {
                V[X(opcode)] = static_cast<uint8_t >(key_idx);
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
        I = SIZEOFSPRITE * V[X(opcode)];
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
    void Chip8::op_regdump(uint16_t opcode) {
        std::copy_n(V.begin(), X(opcode)+1, memory.begin() + I);
    }


    // FX65 - LD Vx, [I]
    // Fills V0 to VX (including VX) with values from memory starting at address I.
    void Chip8::op_regload(uint16_t opcode) {
        std::copy_n(memory.begin() + I, X(opcode)+1, V.begin());
    }


    Chip8::MFP Chip8::fetch_op(uint16_t opcode) {
        static constexpr auto map = Map<uint16_t, MFP, operations.size()>{{operations}};
        const auto idx = get4Bit(opcode, 12);
        const auto mask = masks[idx];
        const auto op = map.at(opcode & mask);
        return op;
    }


    std::array<uint8_t, Chip8::SCREEN_WIDTH * Chip8::SCREEN_HEIGHT> Chip8::getScreen() const {
      std::array<uint8_t, 64 * 32> screen{0};
      std::for_each(
        display_buffer.rbegin(), display_buffer.rend(), [&screen, idx = int{0}](uint8_t byte) mutable {
        std::bitset<8> bits{byte};
        for(int i = 0; i < 8; i++) {
          screen[idx++] = bits[i] ? 255 : 0;
        }
      });
      return screen;
    }


    void Chip8::incPC() {
      static constexpr uint16_t inc = 2;
      PC += inc;
    }


    void Chip8::signal() {
        delay_timer = std::max(delay_timer - 1, 0);
        sound_timer = std::max(sound_timer - 1, 0);
    }


    void Chip8::reset() {
        PC = PC_START_ADDRESS;
        I = 0;
        op_clear_screen(0);

        // clear registers
        std::fill(V.begin(), V.end(), 0);
        // clear stack
        stack = {};
        delay_timer = 0;
        sound_timer = 0;
    }

    } // namespace chip8
#pragma GCC diagnostic pop
