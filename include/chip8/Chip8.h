
#ifndef CHIP8_CHIP8_H
#define CHIP8_CHIP8_H

#include <array>
#include <stack>
#include <string>
#include <vector>
#include <algorithm>

namespace chip8 {

enum class State{ Running, Paused, Reset, Empty };

/**
* Chip8 - the class implements a chip8 emulator.
*
*/
class Chip8 {
  public:
    static constexpr auto screen_width = 64;
    static constexpr auto screen_height = 32;
    static constexpr auto screen_size = screen_width * screen_height;
    static constexpr auto mem_size = 4096;
    static constexpr auto num_registers = 16;
    static constexpr auto pc_start_address = 512;
    static constexpr auto num_opcodes = 34;
    static constexpr auto call_stack_size = 40;

    Chip8();

    void load_rom_from_file(const std::string &filename);

    template<typename RangeT>
    void load_rom(const RangeT &rom) {
        std::ranges::copy(rom, memory.begin() + pc_start_address);
        program_size = rom.size();
        reset();
    }

    void reset_rom();

    /**
     * Execute one op cycle:
     * Fetch opcode, execute operation and increase the program counter.
     */
    void exec_op_cycle();
    /**
     *
     */
    void signal();

    void tick();

    /**
     * Get the Chip8 display buffer as a continuous array of size screen_size,
     * with a field for every pixel of the Chip8 display. If a pixel is set,
     * the corresponding array field is set to 0xFF otherwise to 0x00.
     *
     * @return Chip8 display as an array
     */
    [[nodiscard]] std::array<uint8_t, screen_size> get_screen() const;
    /**
     * Reference to the display buffer. Every bit corresponds to a pixel, grouped by 8 in uint8.
     *
     * @return display buffer
     */
    [[nodiscard]] const std::array<uint8_t, screen_height * 8> &get_display_buffer() const;
    /**
     * Current state of the emulator.
     *
     * @return Chip8 state
     */
    [[nodiscard]] State get_state() const { return state; }
    /**
     * The Chip8-sound signal has only two states: On or Off.
     *
     * @return
     */
    [[nodiscard]] bool sound_signal() const { return sound_timer != 0; };
    /**
     * Pause or unpause a running Chip8 program.
     */
    void toggle_pause();

    [[nodiscard]] uint16_t get_pc() const { return PC; }
    [[nodiscard]] uint16_t get_i() const { return I; }
    [[nodiscard]] uint16_t get_delay_timer() const { return delay_timer; }
    [[nodiscard]] uint16_t get_sound_timer() const { return sound_timer; }
    [[nodiscard]] std::size_t get_tick_count() const { return tick_count; }
    [[nodiscard]] const std::array<uint8_t, mem_size> &get_memory() const { return memory; }
    [[nodiscard]] const std::array<uint8_t, num_registers> &get_registers() const { return V; }
    [[nodiscard]] const std::deque<uint16_t> &get_call_stack() const { return call_stack; }

    std::array<bool, 16> keys{};
    int cycles_per_frame = 8;
    bool draw_flag = false;

    /**
     * Choose implementation of shift operations
     *
     * There are differing interpretations of how the shift operations (8XY6, 8XYE) should be implemented:
     * either shift the value of register Vx or Vy. Some Chip8 programs assume one or the other implementation.
     * This option allows to switch between implementations.
     *
     * See: https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set
     *
     * @param shift_vy If true Vy is shifted, otherwise Vx.
     */
    void set_shift_implementation(bool shift_vy);

  private:
    using MFP = void (Chip8::*)(uint16_t);

    State state = State::Empty;
    std::size_t program_size = 0;

    uint16_t PC = pc_start_address;
    uint16_t I = 0;

    std::array<uint8_t, num_registers> V{}; // 16 general purpose registers (VF register is used as flag)
    std::array<uint8_t, mem_size> memory{};
    std::stack<uint16_t> stack{};
    uint8_t delay_timer{};    // DT
    uint8_t sound_timer{};    // ST

    std::array<uint8_t, 8 * screen_height> display_buffer{}; // NOLINT no overflow

    bool shift_implementation_vy = true;
    std::deque<uint16_t> call_stack;
    std::size_t tick_count = 0;

    void reset();
    void error();

    [[nodiscard]] static MFP fetch_op(uint16_t opcode);
    void incPC();
    // Operations
    void op_clear_screen(uint16_t opcode);
    void op_return_from_subroutine(uint16_t opcode);
    void op_goto(uint16_t opcode);
    void op_call_subroutine(uint16_t opcode);
    void op_skip_ifeq_vx_nn(uint16_t opcode);
    void op_skip_ifneq_vx_nn(uint16_t opcode);
    void op_skip_ifeq_xy(uint16_t opcode);
    void op_skip_if_key_vx_pressed(uint16_t opcode);
    void op_skip_if_key_vx_not_pressed(uint16_t opcode);
    void op_ld_vx_nn(uint16_t opcode);
    void op_add_vx_nn(uint16_t opcode);
    void op_ld_vx_vy(uint16_t opcode);
    void op_or_vx_vy(uint16_t opcode);
    void op_and_vx_vy(uint16_t opcode);
    void op_xor_vx_vy(uint16_t opcode);
    void op_add_vx_vy(uint16_t opcode);
    void op_sub_vx_vy(uint16_t opcode);
    void op_rshift(uint16_t opcode);
    void op_sub_vx_vy_minus_vx(uint16_t opcode);
    void op_lshift(uint16_t opcode);
    void op_skip_ifneq_xy(uint16_t opcode);
    void op_ld_i_nnn(uint16_t opcode);
    void op_goto_I_plus_v0(uint16_t opcode);
    void op_and_rand(uint16_t opcode);
    void op_draw(uint16_t opcode);
    void op_ld_vx_delay_timer(uint16_t opcode);
    void op_get_key_pressed(uint16_t opcode);
    void op_ld_delay_timer_vx(uint16_t opcode);
    void op_ld_sound_timer_vx(uint16_t opcode);
    void op_add_to_I(uint16_t opcode);
    void op_set_I_to_digit_sprite_address(uint16_t opcode);
    void op_vx_to_BCD(uint16_t opcode);
    void op_regdump(uint16_t opcode);
    void op_regload(uint16_t opcode);

    static constexpr std::array<std::pair<uint16_t, MFP>, num_opcodes> operations{
        {
                { 0x00E0, &Chip8::op_clear_screen }, { 0x00EE, &Chip8::op_return_from_subroutine },
                { 0x1000, &Chip8::op_goto }, { 0x2000, &Chip8::op_call_subroutine },
                { 0x3000, &Chip8::op_skip_ifeq_vx_nn }, {0x4000, &Chip8::op_skip_ifneq_vx_nn },
                { 0x5000, &Chip8::op_skip_ifeq_xy }, { 0x6000, &Chip8::op_ld_vx_nn },
                { 0x7000, &Chip8::op_add_vx_nn }, {0x8000, &Chip8::op_ld_vx_vy },
                { 0x8001, &Chip8::op_or_vx_vy }, {0x8002, &Chip8::op_and_vx_vy }, {0x8003, &Chip8::op_xor_vx_vy },
                { 0x8004, &Chip8::op_add_vx_vy }, {0x8005, &Chip8::op_sub_vx_vy }, {0x8006, &Chip8::op_rshift },
                { 0x8007, &Chip8::op_sub_vx_vy_minus_vx }, {0x800E, &Chip8::op_lshift },
                { 0x9000, &Chip8::op_skip_ifneq_xy }, { 0xA000, &Chip8::op_ld_i_nnn },
                { 0xB000, &Chip8::op_goto_I_plus_v0 }, {0xC000, &Chip8::op_and_rand },
                { 0xD000, &Chip8::op_draw }, { 0xE09E, &Chip8::op_skip_if_key_vx_pressed },
                { 0xE0A1, &Chip8::op_skip_if_key_vx_not_pressed }, {0xF007, &Chip8::op_ld_vx_delay_timer },
                { 0xF00A, &Chip8::op_get_key_pressed }, {0xF015, &Chip8::op_ld_delay_timer_vx },
                { 0xF018, &Chip8::op_ld_sound_timer_vx }, {0xF01E, &Chip8::op_add_to_I },
                { 0xF029, &Chip8::op_set_I_to_digit_sprite_address }, {0xF033, &Chip8::op_vx_to_BCD },
                { 0xF055, &Chip8::op_regdump }, { 0xF065, &Chip8::op_regload },
        }
    };
};

}// namespace chip8

#endif// CHIP8_CHIP8_H
