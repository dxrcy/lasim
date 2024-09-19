#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <cstdlib>

#define MEMORY_SIZE 0x10000L    // Total amount of WORDS in ENTIRE memory
#define MEMORY_USER_MAX 0xFDFF  // Index of last WORD in user program area

#define GP_REGISTER_COUNT 8  // Amount of general purpose registers

#define CONDITION_NEGATIVE 0b100
#define CONDITION_ZERO 0b010
#define CONDITION_POSITIVE 0b001
#define CONDITION_DEFAULT CONDITION_ZERO  // Value on program start

#define WORD_SIZE sizeof(Word)
// All 1's for sizeof(Word)
#define WORD_MAX_UNSIGNED ((Word)(~0))

// Swap high and low bytes of a word
#define swap_endian(_word) (((_word) << 8) | ((_word) >> 8))

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

    // 3 bits, NZP
    ConditionCode condition = CONDITION_DEFAULT;
} Registers;

// 4 bits
enum class Opcode {
    ADD = 0b0001,
    AND = 0b0101,
    NOT = 0b1001,
    BR = 0b0000,
    JMP_RET = 0b1100,
    JSR_JSRR = 0b0100,
    LD = 0b0010,
    ST = 0b0011,
    LDI = 0b1010,
    STI = 0b1011,
    LDR = 0b0110,
    STR = 0b0111,
    LEA = 0b1110,
    TRAP = 0b1111,
    RTI = 0b1000,
    RESERVED = 0b1101,
};

// 8 bits
enum class TrapVector {
    // Standard
    GETC = 0x20,
    OUT = 0x21,
    PUTS = 0x22,
    IN = 0x23,
    PUTSP = 0x24,
    HALT = 0x25,
    // Extension
    REG = 0x27,
};

typedef struct ObjectFile {
    enum {
        FILE,
        MEMORY,
    } kind;
    const char *filename;
} ObjectFile;

#endif
