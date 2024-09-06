#ifndef TYPES_CPP
#define TYPES_CPP

#include <cstdint>
#include <cstdlib>

// Defined in execute.cpp
void free_memory(void);

#define ERR_ARGS 0x10
#define ERR_FILE 0x20
#define ERR_MALFORMED_INSTR 0x30
#define ERR_UNIMPLEMENTED 0x40
#define ERR_BAD_ADDRESS 0x50

#define MEMORY_SIZE 0x10000L  // Total amount of allocated WORDS in memory
#define GP_REGISTER_COUNT 8   // Amount of general purpose registers

#define BITS_LOW_1 0b0000'0000'0000'0001
#define BITS_LOW_2 0b0000'0000'0000'0011
#define BITS_LOW_3 0b0000'0000'0000'0111
#define BITS_LOW_4 0b0000'0000'0000'1111
#define BITS_LOW_5 0b0000'0000'0001'1111
#define BITS_LOW_6 0b0000'0000'0011'1111
#define BITS_LOW_8 0b0000'0000'1111'1111
#define BITS_LOW_9 0b0000'0001'1111'1111
#define BITS_LOW_11 0b0000'0111'1111'1111

#define BITS_HIGH_9 0b1111'1111'1000'0000

#define CONDITION_NEGATIVE 0b100
#define CONDITION_ZERO 0b010
#define CONDITION_POSITIVE 0b001

#define WORD_SIZE sizeof(Word)

// For `ADD` and `AND` instructions
#define ARITH_IS_IMMEDIATE(instr) (bool)((((instr) >> 5) & 0b1) != 0b0)

#define EXIT(code)     \
    {                  \
        free_memory(); \
        exit(code);    \
    }

#define to_signed_word(value, size) \
    (sign_extend(static_cast<SignedWord>(value), size))

#define low_6_bits_signed(instr) (to_signed_word((instr) & BITS_LOW_6, 6))
#define low_9_bits_signed(instr) (to_signed_word((instr) & BITS_LOW_9, 9))
#define low_11_bits_signed(instr) (to_signed_word((instr) & BITS_LOW_11, 11))

typedef uint16_t Word;  // 2 bytes
typedef int16_t SignedWord;

typedef uint8_t Register;    // 3 bits
typedef uint8_t Condition3;  // 3 bits

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
    Condition3 condition = 0b010;
} Registers;

#endif
