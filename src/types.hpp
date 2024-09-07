#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <cstdlib>

#define MEMORY_SIZE 0x10000L  // Total amount of WORDS in ENTIRE memory
#define GP_REGISTER_COUNT 8   // Amount of general purpose registers

#define CONDITION_NEGATIVE 0b100
#define CONDITION_ZERO 0b010
#define CONDITION_POSITIVE 0b001
#define CONDITION_DEFAULT CONDITION_ZERO  // Value on program start

#define WORD_SIZE sizeof(Word)

typedef uint16_t Word;       // 2 bytes
typedef int16_t SignedWord;  // 2 bytes

typedef uint8_t Register;       // 3 bits
typedef uint8_t ConditionCode;  // 3 bits

typedef struct Registers {
    // As long as there are 8 GP registers, and a register operand is defined
    // with 3 bits, then a properly created `Register` integer may be used to
    // index this array without worry.
    // ^ This is written very strangly for some reason.
    Word general_purpose[GP_REGISTER_COUNT] = {0};

    Word program_counter;
    Word stack_pointer;
    Word frame_pointer;

    // 3 bits, NZP
    ConditionCode condition = CONDITION_DEFAULT;
} Registers;

// 4 bits
typedef enum Opcode {
    OPCODE_ADD = 0b0001,
    OPCODE_AND = 0b0101,
    OPCODE_BR = 0b0000,
    OPCODE_JMP_RET = 0b1100,
    OPCODE_JSR_JSRR = 0b0100,
    OPCODE_LD = 0b0010,
    OPCODE_LDI = 0b1010,
    OPCODE_LDR = 0b0110,
    OPCODE_LEA = 0b1110,
    OPCODE_NOT = 0b1001,
    OPCODE_RTI = 0b1000,
    OPCODE_ST = 0b0011,
    OPCODE_STI = 0b1011,
    OPCODE_STR = 0b0111,
    OPCODE_TRAP = 0b1111,
    OPCODE_RESERVED = 0b1101,
} Opcode;

// 8 bits
enum TrapVector {
    TRAP_GETC = 0x20,
    TRAP_OUT = 0x21,
    TRAP_PUTS = 0x22,
    TRAP_IN = 0x23,
    TRAP_PUTSP = 0x24,
    TRAP_HALT = 0x25,
};

#endif
