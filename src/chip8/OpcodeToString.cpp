#include "chip8/OpcodeToString.h"

#include <string>

#include <fmt/core.h>

#include "chip8/InstructionPartAccessorFunctions.h"

namespace chip8 {

    std::string opcode_to_assembler_formatted(uint16_t opcode) {
        const uint16_t mask = masks[(opcode >> 12) & 0xFU];
        switch (opcode & mask) {
            case 0x00E0: return ""; // NOLINT identical branches intentional
            case 0x00EE: return "";
            case 0x1000: return fmt::format("   0x{:03x}", nnn(opcode));
            case 0x2000: return fmt::format("     0x{:03x}", nnn(opcode));
            case 0x3000: return fmt::format("   V{:x}, 0x{:02x}", X(opcode), nn(opcode));
            case 0x4000: return fmt::format("    V{:x}, 0x{:02x}", X(opcode), nn(opcode));
            case 0x5000: return fmt::format("   V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x6000: return fmt::format("   V{:x}, 0x{:02x}", X(opcode), nn(opcode));
            case 0x7000: return fmt::format("    V{:x}, 0x{:02x}", X(opcode), nn(opcode));
            case 0x8000: return fmt::format("   V{:x}, V{:x}", X(opcode), Y(opcode)); // NOLINT identical branches intentional
            case 0x8001: return fmt::format("   V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x8002: return fmt::format("    V{:x}, V{:x}", X(opcode), Y(opcode));  // NOLINT identical branches intentional
            case 0x8003: return fmt::format("    V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x8004: return fmt::format("    V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x8005: return fmt::format("    V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x8006: return fmt::format("    V{:x} [, V{:x}]", X(opcode), Y(opcode));
            case 0x8007: return fmt::format("     V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0x800E: return fmt::format("    V{:x} [, V{:x}]", X(opcode), Y(opcode));
            case 0x9000: return fmt::format("    V{:x}, V{:x}", X(opcode), Y(opcode));
            case 0xA000: return fmt::format("   I, 0x{:03x}", nnn(opcode));
            case 0xB000: return fmt::format("   V0, 0x{:03x}", nnn(opcode));
            case 0xC000: return fmt::format("    V{:x}, 0x{:02x}", X(opcode), nn(opcode));
            case 0xD000: return fmt::format("    V{:x}, V{:x}, {:x}", X(opcode), Y(opcode), n(opcode));
            case 0xE09E: return fmt::format("    V{:x}", X(opcode));
            case 0xE0A1: return fmt::format("     V{:x}", X(opcode));
            case 0xF007: return fmt::format("   V{:x}, DelayTimer", X(opcode));
            case 0xF00A: return fmt::format("   V{:x}, Key", X(opcode));
            case 0xF015: return fmt::format("   DelayTimer, V{:x}", X(opcode));
            case 0xF018: return fmt::format("   SoundTimer, V{:x}", X(opcode));
            case 0xF01E: return fmt::format("    I, V{:x}", X(opcode));
            case 0xF029: return fmt::format("   I,                     V{:x}", X(opcode));
            case 0xF033: return fmt::format("    V{:x}", X(opcode));
            case 0xF055: return fmt::format("               [I], V{:x}", X(opcode));
            case 0xF065: return fmt::format("               V{:x}, [I]", X(opcode));
            default: return "";
        }
    }

    std::string opcode_to_assembler_help_text(uint16_t opcode) {
        const uint16_t mask = masks[(opcode >> 12) & 0xFU];
        switch (opcode & mask) {
            case 0x00E0: return "Clear the screen";
            case 0x00EE: return "Return from a subroutine";
            case 0x1000: return "Jump to address nnn";
            case 0x2000: return "Execute subroutine at nnn";
            case 0x3000: return "Skip the following instruction if the value of register Vx equals nn";
            case 0x4000: return "Skip the following instruction if the value of register Vx is not equal to nn";
            case 0x5000: return "Skip the following instruction if the value of register Vx is equal to the value of register Vy";
            case 0x6000: return "Store number nn in register Vx";
            case 0x7000: return "Add the value nn to register Vx";
            case 0x8000: return "Store the value of register Vy in register Vx";
            case 0x8001: return "Set Vx to Vx OR Vy";
            case 0x8002: return "Set Vx to Vx AND Vy";
            case 0x8003: return "Set Vx to Vx XOR Vy";
            case 0x8004: return "Add the value of register Vy to register Vx. Set VF to 0x01 if a carry occurs. Set VF to 0x00 if a carry does not occur";
            case 0x8005: return "Subtract the value of register Vy from register Vx. Set VF to 0x00 if a borrow occurs. Set VF to 0x01 if a borrow does not occur";
            case 0x8006: return "Store the value of register Vy shifted right one bit in register Vx. Set register VF to the least significant bit prior to the shift. Vy is unchanged.\n Some ROMs assume a different implementation shifting Vx rather than Vy. See Settings to switch between implementations.";
            case 0x8007: return "Set register Vx to the value of Vy minus Vx. Set VF to 0x00 if a borrow occurs. Set VF to 0x01 if a borrow does not occur";
            case 0x800E: return "Store the value of register Vy shifted left one bit in register Vx. Set register VF to the most significant bit prior to the shift. Vy is unchanged.\n Some ROMs assume a different implementation shifting Vx rather than Vy. See Settings to switch between implementations.";
            case 0x9000: return "Skip the following instruction if the value of register Vx is not equal to the value of register Vy.";
            case 0xA000: return "Store memory address nnn in register I.";
            case 0xB000: return "Jump to address nnn + V0";
            case 0xC000: return "Set Vx to a random number with a mask of nn";
            case 0xD000: return "Draw a sprite at position Vx, Vy with n bytes of sprite data starting at the address stored in I. Set VF to 01 if any set pixels are changed to unset, and 00 otherwise";
            case 0xE09E: return "Skip the following instruction if the key corresponding to the hex value currently stored in register Vx is pressed.";
            case 0xE0A1: return "Skip the following instruction if the key corresponding to the hex value currently stored in register Vx is not pressed";
            case 0xF007: return "Store the current value of the delay timer in register Vx";
            case 0xF00A: return "Wait for a keypress and store the result in register Vx";
            case 0xF015: return "Set the delay timer to the value of register Vx";
            case 0xF018: return "Set the sound timer to the value of register Vx";
            case 0xF01E: return "Add the value stored in register Vx to register I";
            case 0xF029: return "Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register Vx";
            case 0xF033: return "Store the binary-coded decimal equivalent of the value stored in register Vx at addresses I, I + 1, and I + 2";
            case 0xF055: return "Store the values of registers V0 to Vx inclusive in memory starting at address I. I is set to I + X + 1 after operation";
            case 0xF065: return "Fill registers V0 to Vx inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation";
            default: return "";
        }
    }

}
