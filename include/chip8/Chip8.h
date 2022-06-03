
#ifndef CHIP8_CHIP8_H
#define CHIP8_CHIP8_H

#include <array>
#include <string>
#include <stack>
#include <bitset>

namespace chip8 {


class Chip8 {
  public:
    static constexpr auto SCREEN_WIDTH = 64;
    static constexpr auto SCREEN_HEIGHT = 32;
    static constexpr auto SCREEN_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT;
    static constexpr auto MEMSIZE = 4096;
    static constexpr auto N_REGISTERS = 16;
    static constexpr auto PC_START_ADDRESS = 512;
    static constexpr auto N_OPCODES = 34;

    Chip8();

    void load_rom(const std::string &path);

    void exec_op_cycle();
    void signal();

    void tick();

    /**
     * Get the Chip8 display buffer as a continuous array of size SCREEN_SIZE,
     * with a field for every pixel of the Chip8 display. If a pixel is set,
     * the corresponding array field is set to 0xFF otherwise to 0x00.
     *
     * @return Chip8 display as an array
     */
    [[nodiscard]] std::array<uint8_t, SCREEN_SIZE> getScreen() const;
    [[nodiscard]] bool game_running() const { return is_running; };

    /**
     * Pause or unpause a running Chip8 program.
     */
    void toggle_pause();

    // TODO consistent naming
    [[nodiscard]] uint16_t getPC() const { return PC; }
    [[nodiscard]] uint16_t getI() const { return I; }
    [[nodiscard]] uint16_t getDT() const { return delay_timer; }
    [[nodiscard]] const std::array<uint8_t, MEMSIZE> &get_memory() const { return memory; }
    [[nodiscard]] const std::array<uint8_t, N_REGISTERS> &get_registers() const { return V; }

    std::array<bool, 16> keys{};
    // TODO changed from uint64_t, and why is this public ???
    int program_size = 0;
    int cycles_per_frame = 8;

    bool draw_flag = false;
  private:
    using MFP = void (Chip8::*)(uint16_t);

    bool is_running = false;
    bool game_loaded = false;

    uint16_t PC = PC_START_ADDRESS;
    uint16_t I = 0;

    std::array<uint8_t, N_REGISTERS> V{}; // 16 general purpose registers (VF register is used as flag)
    std::array<uint8_t, MEMSIZE> memory{};
    std::stack<uint16_t> stack = std::stack<uint16_t>();
    uint8_t delay_timer{};    // DT
    uint8_t sound_timer{};    // ST

    std::array<uint8_t, 8 * 32> display_buffer{};
    std::array<uint8_t, 8 * 32> display_buffer_backup{};

    void reset();

    [[nodiscard]] static MFP fetch_op(uint16_t opcode);
    void incPC();
    // Operations
    void op_clear_screen(uint16_t opcode);
    void op_return_from_subroutine(uint16_t opcode);
    void op_goto(uint16_t opcode);
    void op_call_subroutine(uint16_t opcode);
    void op_skip_ifeq(uint16_t opcode);
    void op_skip_ifneq(uint16_t opcode);
    void op_skip_ifeq_xy(uint16_t opcode);
    void op_keyop1(uint16_t opcode);
    void op_keyop2(uint16_t opcode);
    void ld_vx_nn(uint16_t opcode);
    void op_add_nn(uint16_t opcode);
    void op_assign(uint16_t opcode);
    void op_or(uint16_t opcode);
    void op_and(uint16_t opcode);
    void op_xor(uint16_t opcode);
    void op_add(uint16_t opcode);
    void op_sub(uint16_t opcode);
    void op_rshift(uint16_t opcode);
    void op_sub_rev(uint16_t opcode);
    void op_lshift(uint16_t opcode);
    void op_skip_ifneq_xy(uint16_t opcode);
    void op_set_i(uint16_t opcode);
    void op_goto_plus_reg(uint16_t opcode);
    void op_and_rand(uint16_t opcode);
    void op_draw(uint16_t opcode);
    void op_to_delay_timer(uint16_t opcode);
    void op_get_key(uint16_t opcode);
    void op_set_delay_timer(uint16_t opcode);
    void op_set_sound_timer(uint16_t opcode);
    void op_add_to_I(uint16_t opcode);
    void op_set_I_to_sprite_address(uint16_t opcode);
    void op_set_BCD(uint16_t opcode);
    void op_regdump(uint16_t opcode);
    void op_regload(uint16_t opcode);

    static constexpr std::array<std::pair<uint16_t, MFP>, N_OPCODES> operations{
        {
            { 0x00E0, &Chip8::op_clear_screen }, { 0x00EE, &Chip8::op_return_from_subroutine },
              { 0x1000, &Chip8::op_goto }, { 0x2000, &Chip8::op_call_subroutine },
              { 0x3000, &Chip8::op_skip_ifeq }, { 0x4000, &Chip8::op_skip_ifneq },
              { 0x5000, &Chip8::op_skip_ifeq_xy }, { 0x6000, &Chip8::ld_vx_nn },
              { 0x7000, &Chip8::op_add_nn }, { 0x8000, &Chip8::op_assign },
              { 0x8001, &Chip8::op_or }, { 0x8002, &Chip8::op_and }, { 0x8003, &Chip8::op_xor },
              { 0x8004, &Chip8::op_add }, { 0x8005, &Chip8::op_sub }, { 0x8006, &Chip8::op_rshift },
              { 0x8007, &Chip8::op_sub_rev }, { 0x800E, &Chip8::op_lshift },
              { 0x9000, &Chip8::op_skip_ifneq_xy }, { 0xA000, &Chip8::op_set_i },
              { 0xB000, &Chip8::op_goto_plus_reg }, { 0xC000, &Chip8::op_and_rand },
              { 0xD000, &Chip8::op_draw }, { 0xE09E, &Chip8::op_keyop1 },
              { 0xE0A1, &Chip8::op_keyop2 }, { 0xF007, &Chip8::op_to_delay_timer },
              { 0xF00A, &Chip8::op_get_key }, { 0xF015, &Chip8::op_set_delay_timer },
              { 0xF018, &Chip8::op_set_sound_timer }, { 0xF01E, &Chip8::op_add_to_I },
              { 0xF029, &Chip8::op_set_I_to_sprite_address }, { 0xF033, &Chip8::op_set_BCD },
              { 0xF055, &Chip8::op_regdump }, { 0xF065, &Chip8::op_regload },
        }
    };
};

}// namespace chip8

#endif// CHIP8_CHIP8_H
