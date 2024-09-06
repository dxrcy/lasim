#include <termios.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ERR_ARGS 0x10
#define ERR_FILE 0x20
#define ERR_MALFORMED_INSTR 0x30
#define ERR_UNIMPLEMENTED 0x40

#define MEMORY_SIZE 0x10000L  // Total amount of allocated WORDS in memory
#define GP_REGISTER_COUNT 8   // Amount of general purpose registers

#define BITS_LOW_2 0b0000'0000'0000'0011
#define BITS_LOW_3 0b0000'0000'0000'0111
#define BITS_LOW_4 0b0000'0000'0000'1111
#define BITS_LOW_5 0b0000'0000'0001'1111
#define BITS_LOW_8 0b0000'0000'1111'1111
#define BITS_LOW_9 0b0000'0001'1111'1111
#define BITS_HIGH_9 0b1111'1111'1000'0000

#define CONDITION_NEGATIVE 0b100
#define CONDITION_ZERO 0b010
#define CONDITION_POSITIVE 0b001

#define WORD_SIZE sizeof(Word)

// For `ADD` and `AND` instructions
#define ARITH_IS_IMMEDIATE(instr) (bool)(((instr) >> 5) && 0b1)

#define EXIT(code)     \
    {                  \
        free_memory(); \
        exit(code);    \
    }

#define UNIMPLEMENTED_INSTR(instr, name)                                  \
    {                                                                     \
        fprintf(stderr, "Unimplemented instruction: 0b%04x: %s\n", instr, \
                name);                                                    \
        EXIT(ERR_UNIMPLEMENTED);                                          \
    }

#define UNIMPLEMENTED_TRAP(vector, name)                                   \
    {                                                                      \
        fprintf(stderr, "Unimplemented trap vector: 0x%02x: %s\n", vector, \
                name);                                                     \
        EXIT(ERR_UNIMPLEMENTED);                                           \
    }

typedef uint16_t Word;  // 2 bytes
typedef int16_t SignedWord;

typedef uint8_t Register;       // 3 bits
typedef uint8_t Immediate5;     // 5 bits
typedef int16_t SignedOffset9;  // 9 bits
typedef uint8_t Condition3;     // 3 bits

// 4 bits
typedef enum Opcode {
    OPCODE_ADD = 0b0001,
    OPCODE_AND = 0b0101,
    OPCODE_BR = 0b0000,
    OPCODE_JMP_RET = 0b1100,
    OPCODE_JSR_JSRR_RTI = 0b0100,
    OPCODE_LD = 0b0010,
    OPCODE_LDI = 0b1010,
    OPCODE_LDR = 0b0110,
    OPCODE_LEA = 0b1110,
    OPCODE_NOT = 0b1001,
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

// Exists for program lifetime, but must still be deleted before exit
// Use `EXIT` macro to automatically free before exiting
// Dynamically allocated due to large size
static Word *memory = new Word[MEMORY_SIZE];
static Registers registers;

void free_memory(void);
Word swap_endianess(const Word word);
void read_file_to_memory(const char *const filename, Word &start, Word &end);
void dbg_print_registers(void);
bool execute_trap_instruction(const Word instr);
bool execute_next_instrution(void);

void tty_nobuffer_noecho(void);
void tty_nobuffer_yesecho(void);
void tty_restore(void);

int main(const int argc, const char *const *const argv) {
    if (argc != 2 || argv[1][0] == '-') {
        fprintf(stderr, "USAGE: lc3sim [FILE]\n");
        EXIT(ERR_ARGS);
    }
    const char *filename = argv[1];

    Word file_start, file_end;
    read_file_to_memory(filename, file_start, file_end);

    /* for (size_t i = file_start - 2; i < file_end + 2; ++i) { */
    /*     printf("FILE: 0x%04lx: 0x%04hx\n", i, memory[i]); */
    /* } */

    // GP and condition registers are already initialized to 0
    registers.program_counter = file_start;
    registers.stack_pointer = file_end;
    registers.frame_pointer = file_end;

    // This loop is written poorly, as I will probably refactor it later.
    while (true) {
        const bool should_halt = execute_next_instrution();
        if (should_halt) {
            break;
        }
    }

    free_memory();
    return 0;
}

// Does not need to be called when using `EXIT` macro
void free_memory() {
    delete[] memory;
    memory = nullptr;
}

// Swap high and low bytes of a word
Word swap_endianess(const Word word) { return (word << 8) | (word >> 8); }

void read_file_to_memory(const char *const filename, Word &start, Word &end) {
    FILE *const file = fopen(filename, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        EXIT(ERR_FILE);
    }

    // TODO: Handle failure
    fread(reinterpret_cast<char *>(&start), WORD_SIZE, 1, file);

    /* printf("origin: 0x%04x\n", start); */

    // TODO: Handle failure
    char *memory_at_file = reinterpret_cast<char *>(memory + start);
    size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    size_t words_read = fread(memory_at_file, WORD_SIZE, max_file_bytes, file);

    end = start + words_read;

    // Mark undefined bytes for debugging
    memset(memory, 0xdd, start * WORD_SIZE);                      // Before file
    memset(memory + end, 0xee, (MEMORY_SIZE - end) * WORD_SIZE);  // After file

    // TODO: Make this better !!
    for (size_t i = start; i < end; ++i) {
        memory[i] = swap_endianess(memory[i]);
    }

    /* printf("words read: %ld\n", words_read); */

    fclose(file);
}

void dbg_print_registers() {
    printf("--------------------------\n");
    printf("    PC  0x%04hx\n", registers.program_counter);
    printf("    SP  0x%04hx\n", registers.stack_pointer);
    printf("    FP  0x%04hx\n", registers.frame_pointer);
    printf("..........................\n");
    printf("    N=%b  Z=%b  P=%b\n",
           (registers.condition >> 2),        // Negative
           (registers.condition >> 2) & 0b1,  // Zero
           (registers.condition >> 2) & 0b1   // Positive
    );
    printf("..........................\n");
    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        const Word value = registers.general_purpose[reg];
        printf("    R%d  0x%04hx  %3d\n", reg, value, value);
    }
    printf("--------------------------\n");
}

void update_condition_codes(Word result) {
    const bool is_negative = result >> 15 == 1;
    const bool is_zero = result == 0;
    const bool is_positive = !is_negative && !is_zero;
    // Set low 3 bits as N,Z,P
    registers.condition = (is_negative << 2) | (is_zero << 1) | is_positive;
}

// `true` return value indicates that program should end
bool execute_next_instrution() {
    const Word instr = memory[registers.program_counter];
    ++registers.program_counter;

    /* printf("INSTR: 0x%04x  %16b\n", instr, instr); */

    // May be invalid enum variant
    // Handled in default switch branch
    const Opcode opcode = static_cast<Opcode>(instr >> 12);

    switch (opcode) {
        // ADD+
        case OPCODE_ADD: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register src_reg1 = (instr >> 6) & BITS_LOW_3;

            const SignedWord value1 =
                static_cast<SignedWord>(registers.general_purpose[src_reg1]);
            SignedWord value2;

            if (!ARITH_IS_IMMEDIATE(instr)) {
                // 2 bits padding
                const uint8_t padding = (instr >> 3) & BITS_LOW_2;
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0x00 for ADD instruction\n");
                    EXIT(ERR_MALFORMED_INSTR);
                }
                const Register src_reg2 = instr & BITS_LOW_3;
                value2 = static_cast<SignedWord>(
                    memory[registers.general_purpose[src_reg2]]);
            } else {
                // TODO: Is this properly sign-extended ??
                const Immediate5 imm = instr & BITS_LOW_5;
                value2 = static_cast<SignedWord>(imm);
            }

            printf(">ADD R%d = R%d + 0x%04hx\n", dest_reg, src_reg1, value2);

            const Word result = static_cast<Word>(value1 + value2);
            registers.general_purpose[dest_reg] = result;

            dbg_print_registers();

            update_condition_codes(result);
        }; break;

        // AND+
        case OPCODE_AND: {
            UNIMPLEMENTED_INSTR(instr, "AND");
        }; break;

        // NOT+
        case OPCODE_NOT: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register src_reg1 = (instr >> 6) & BITS_LOW_3;

            // 4 bits padding
            const uint8_t padding = instr & BITS_LOW_4;
            if (padding != 0b1111) {
                fprintf(stderr, "Expected padding 0xf for NOT instruction\n");
                EXIT(ERR_MALFORMED_INSTR);
            }

            printf(">NOT R%d = NOT R%d\n", dest_reg, src_reg1);

            const Word result = ~registers.general_purpose[src_reg1];
            registers.general_purpose[dest_reg] = result;

            dbg_print_registers();

            update_condition_codes(result);
        }; break;

        // BRcc
        case OPCODE_BR: {
            // Skip special NOP case
            if (instr == 0x0000) {
                break;
            }

            printf("0x%04x\t0b%016b\n", instr, instr);
            Condition3 condition = (instr >> 9) & BITS_LOW_3;
            SignedOffset9 pc_offset = instr & BITS_LOW_9;

            printf("BR: %03b & %03b = %03b -> %b\n", condition,
                   registers.condition, condition & registers.condition,
                   (condition & registers.condition) != 0b000);

            // If any bits of the condition codes match
            if ((condition & registers.condition) != 0b000) {
                registers.program_counter += pc_offset;
            }
            printf("branched to 0x%04x\n", registers.program_counter);
        }; break;

        // JMP/RET
        case OPCODE_JMP_RET: {
            UNIMPLEMENTED_INSTR(instr, "JMP/RET");
        }; break;

        // JSR/JSRR/RTI
        case OPCODE_JSR_JSRR_RTI: {
            UNIMPLEMENTED_INSTR(instr, "JSR/JSRR/RTI");
        }; break;

        // LD+
        case OPCODE_LD: {
            UNIMPLEMENTED_INSTR(instr, "LD");
        }; break;

        // ST
        case OPCODE_ST: {
            UNIMPLEMENTED_INSTR(instr, "ST");
        }; break;

        // LDI+
        case OPCODE_LDI: {
            UNIMPLEMENTED_INSTR(instr, "LDI");
        }; break;

        // STI
        case OPCODE_STI: {
            UNIMPLEMENTED_INSTR(instr, "STI");
        }; break;

        // LDR+
        case OPCODE_LDR: {
            UNIMPLEMENTED_INSTR(instr, "LDR");
        }; break;

        // STR
        case OPCODE_STR: {
            UNIMPLEMENTED_INSTR(instr, "STR");
        }; break;

        // LEA+
        case OPCODE_LEA: {
            const Register dest_reg = (instr >> 9) & 0b111;
            const SignedOffset9 pc_offset = instr & BITS_LOW_9;
            /* printf(">LEA REG%d, pc_offset:0x%04hx\n", reg, */
            /*        pc_offset); */
            /* print_registers(); */
            registers.general_purpose[dest_reg] =
                registers.program_counter + pc_offset;
            dbg_print_registers();
        }; break;

        // TRAP
        case OPCODE_TRAP:
            if (execute_trap_instruction(instr)) {
                return true;
            }
            break;

        // (reserved)
        case OPCODE_RESERVED:
            fprintf(stderr, "Invalid reserved opcode: 0x%04x\n", opcode);
            EXIT(ERR_MALFORMED_INSTR);
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0x%04x\n", opcode);
            EXIT(ERR_MALFORMED_INSTR);
    }

    return false;
}

// TODO: Move to another file
static struct termios tty;
void tty_get() { tcgetattr(STDIN_FILENO, &tty); }
void tty_apply() { tcsetattr(STDIN_FILENO, TCSANOW, &tty); }
void tty_nobuffer_noecho() {
    tty_get();
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty_apply();
}
void tty_nobuffer_yesecho() {
    tty_get();
    tty.c_lflag &= ~ICANON;
    tty.c_lflag |= ECHO;
    tty_apply();
}
void tty_restore() {
    tty.c_lflag |= ICANON;
    tty.c_lflag |= ECHO;
    tty_apply();
}

// `true` return value indicates that program should end
bool execute_trap_instruction(const Word instr) {
    // 4 bits padding
    const uint8_t padding = (instr >> 8) & BITS_LOW_4;
    if (padding != 0b0000) {
        fprintf(stderr, "Expected padding 0x00 for TRAP instruction\n");
        EXIT(ERR_MALFORMED_INSTR);
    }

    // May be invalid enum variant
    // Handled in default switch branch
    const enum TrapVector trap_vector =
        static_cast<enum TrapVector>(instr & BITS_LOW_9);

    switch (trap_vector) {
        case TRAP_GETC: {
            tty_nobuffer_noecho();
            char input = getchar();
            tty_restore();
            input &= BITS_LOW_8;  // Zero high 8 bits
            registers.general_purpose[0] = input;
        }; break;

        case TRAP_IN: {
            tty_nobuffer_yesecho();
            char input = getchar();
            tty_restore();
            input &= BITS_LOW_8;  // Zero high 8 bits
            registers.general_purpose[0] = input;
        }; break;

        case TRAP_OUT: {
            Word word = registers.general_purpose[0];
            char ch = static_cast<char>(word);
            printf("%c", ch);
        }; break;

        case TRAP_PUTS: {
            const Word *str = &memory[registers.general_purpose[0]];
            for (Word ch; (ch = str[0]) != 0x0000; ++str) {
                if (ch & BITS_HIGH_9) {
                    fprintf(stderr,
                            "String contains non-ASCII characters, "
                            "which are not supported.");
                    EXIT(ERR_UNIMPLEMENTED);
                }
                printf("%c", ch);
            }
        } break;

        case TRAP_PUTSP: {
            const Word *const str_words = &memory[registers.general_purpose[0]];
            const char *str = reinterpret_cast<const char *const>(str_words);
            for (Word ch; (ch = str[0]) != 0x00; ++str) {
                printf("%c", ch);
            }
        }; break;

        case TRAP_HALT: {
            return true;
        }; break;

        default:
            fprintf(stderr, "Invalid trap vector 0x%02x\n", trap_vector);
            EXIT(ERR_MALFORMED_INSTR);
    }

    return false;
}
