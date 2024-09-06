#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstdlib>

#define ERR_CLI_ARGUMENTS 0x10
#define ERR_FILE_READ 0x20
#define ERR_MALFORMED_INSTR 0x30
#define ERR_MALFORMED_PADDING 0x31
#define ERR_BAD_ADDRESS 0x40
#define ERR_UNIMPLEMENTED 0x70

#define MEMORY_SIZE 0x10000L  // Total amount of allocated WORDS in memory
#define GP_REGISTER_COUNT 8   // Amount of general purpose registers

#define WORD_SIZE sizeof(Word)

typedef uint16_t Word;       // 2 bytes
typedef int16_t SignedWord;  // 2 bytes

typedef uint8_t Register;       // 3 bits
typedef uint8_t ConditionCode;  // 3 bits

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

typedef struct Registers {
    Word general_purpose[GP_REGISTER_COUNT] = {0};

    Word program_counter;
    Word stack_pointer;
    Word frame_pointer;

    // 3 bits, NZP, N=0, Z=1, P=0
    ConditionCode condition = 0b010;
} Registers;

#endif
