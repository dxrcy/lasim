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

// Exists for program lifetime, but must still be deleted before exit
// Use `EXIT` macro to automatically free before exiting
// Dynamically allocated due to large size
static Word *memory = new Word[MEMORY_SIZE];
static Registers registers;

void free_memory(void);
Word swap_endianess(const Word word);
void read_file_to_memory(const char *const filename, Word &start, Word &end);
void dbg_print_registers(void);
void set_condition_codes(const Word result);
void print_char(const char ch);
static char *const binary_string_word(const Word word);
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
    Word origin;
    fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, file);
    start = swap_endianess(origin);

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
    printf("    N=%x  Z=%x  P=%x\n",
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

void set_condition_codes(const Word result) {
    const bool is_negative = result >> 15 == 1;
    const bool is_zero = result == 0;
    const bool is_positive = !is_negative && !is_zero;
    // Set low 3 bits as N,Z,P
    registers.condition = (is_negative << 2) | (is_zero << 1) | is_positive;
}

void print_char(const char ch) {
    if (ch == '\r') {
        printf("\n");
    }
    printf("%c", ch);
}

// Since %b printf format specifier is not ISO-compliant
static char *const binary_string_word(const Word word) {
    static char str[5];
    for (int i = 0; i < 4; ++i) {
        str[i] = '0' + ((word >> (3 - i)) & 0b1);
    }
    str[4] = '\0';
    return str;
}

SignedWord sign_extend(SignedWord value, const size_t size) {
    // If previous-highest bit is set
    if (value >> (size - 1) & 0b1) {
        // Set all bits higher than previous sign bit to 1
        return value | (~0 << size);
    }
    return value;
}

// `true` return value indicates that program should end
bool execute_next_instrution() {
    const Word instr = memory[registers.program_counter];
    ++registers.program_counter;

    // printf("INSTR at 0x%04x: 0x%04x  %016b\n", registers.program_counter - 1,
    //        instr, instr);

    if (instr == 0xdddd) {
        fprintf(stderr, "Cannot execute non-user memory (before origin)\n");
        EXIT(ERR_BAD_ADDRESS);
    }
    if (instr == 0xeeee) {
        fprintf(stderr,
                "Cannot execute non-executable memory (after end of file)\n");
        EXIT(ERR_BAD_ADDRESS);
    }

    // May be invalid enum variant
    // Handled in default switch branch
    const Opcode opcode = static_cast<Opcode>(instr >> 12);

    // TODO: Check all operands for whether they need to be sign-extended !!!!

    switch (opcode) {
        // ADD*
        case OPCODE_ADD: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register src_reg_a = (instr >> 6) & BITS_LOW_3;

            const SignedWord value_a =
                static_cast<SignedWord>(registers.general_purpose[src_reg_a]);
            SignedWord value_b;

            if (!ARITH_IS_IMMEDIATE(instr)) {
                // 2 bits padding
                const uint8_t padding = (instr >> 3) & BITS_LOW_2;
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for ADD instruction\n");
                    EXIT(ERR_MALFORMED_INSTR);
                }
                const Register src_reg_b = instr & BITS_LOW_3;
                value_b = static_cast<SignedWord>(
                    registers.general_purpose[src_reg_b]);
            } else {
                value_b = to_signed_word(instr & BITS_LOW_5, 5);
            }

            const Word result = static_cast<Word>(value_a + value_b);

            // printf("\n>ADD R%d = (R%d) 0x%04hx + 0x%04hx = 0x%04hx\n",
            // dest_reg, src_reg_a, value_a, value_b, result);

            registers.general_purpose[dest_reg] = result;

            /* dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // AND*
        case OPCODE_AND: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register src_reg_a = (instr >> 6) & BITS_LOW_3;

            const Word value_a = registers.general_purpose[src_reg_a];
            Word value_b;

            if (!ARITH_IS_IMMEDIATE(instr)) {
                // 2 bits padding
                const uint8_t padding = (instr >> 3) & BITS_LOW_2;
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for AND instruction\n");
                    EXIT(ERR_MALFORMED_INSTR);
                }
                const Register src_reg_b = instr & BITS_LOW_3;
                value_b = memory[registers.general_purpose[src_reg_b]];
            } else {
                value_b = static_cast<SignedWord>(instr & BITS_LOW_5);
            }

            /* printf(">AND R%d = R%d & 0x%04hx\n", dest_reg, src_reg_a,
             * value_b);
             */

            const Word result = value_a & value_b;
            registers.general_purpose[dest_reg] = result;

            /* dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // NOT*
        case OPCODE_NOT: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register src_reg = (instr >> 6) & BITS_LOW_3;

            // 4 bits padding
            const uint8_t padding = instr & BITS_LOW_4;
            if (padding != 0b1111) {
                fprintf(stderr, "Expected padding 0xf for NOT instruction\n");
                EXIT(ERR_MALFORMED_INSTR);
            }

            /* printf(">NOT R%d = NOT R%d\n", dest_reg, src_reg1); */

            const Word result = ~registers.general_purpose[src_reg];
            registers.general_purpose[dest_reg] = result;

            /* dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // BRcc
        case OPCODE_BR: {
            // Skip special NOP case
            if (instr == 0x0000) {
                break;
            }

            // TODO: This might never branch if given CC=0b000 ????

            /* printf("0x%04x\t0b%016b\n", instr, instr); */
            const Condition3 condition = (instr >> 9) & BITS_LOW_3;
            const SignedWord offset = low_9_bits_signed(instr);

            /* printf("BR: %03b & %03b = %03b -> %b\n", condition, */
            /*        registers.condition, condition & registers.condition, */
            /*        (condition & registers.condition) != 0b000); */
            /* printf("PCOffset: 0x%04x  %d\n", offset, offset); */

            // If any bits of the condition codes match
            if ((condition & registers.condition) != 0b000) {
                registers.program_counter += offset;
                /* printf("branched to 0x%04x\n", registers.program_counter); */
            }
        }; break;

        // JMP/RET
        case OPCODE_JMP_RET: {
            // 3 bits padding
            const uint8_t padding_1 = (instr >> 9) & BITS_LOW_3;
            if (padding_1 != 0b000) {
                fprintf(stderr,
                        "Expected padding 0b000 for JMP/RET instruction\n");
                EXIT(ERR_MALFORMED_INSTR);
            }
            // 6 bits padding
            // After base register
            const uint8_t padding_2 = instr & BITS_LOW_6;
            if (padding_2 != 0b000000) {
                fprintf(stderr,
                        "Expected padding 0b000000 for JMP/RET instruction\n");
                EXIT(ERR_MALFORMED_INSTR);
            }
            const Register base_reg = (instr >> 6) & BITS_LOW_3;
            const Word base = registers.general_purpose[base_reg];
            registers.program_counter = base;
        }; break;

        // JSR/JSRR
        case OPCODE_JSR_JSRR: {
            // Save PC to R7
            registers.general_purpose[7] = registers.program_counter;
            // Bit 11 defines JSR or JSRR
            const bool is_offset = ((instr >> 11) & BITS_LOW_1) != 0;
            if (is_offset) {
                // JSR
                const SignedWord offset = low_11_bits_signed(instr);
                /* printf("JSR: PCOffset = 0x%04x\n", pc_offset); */
                registers.program_counter += offset;
            } else {
                // JSRR
                // 2 bits padding
                const uint8_t padding = (instr >> 9) & BITS_LOW_2;
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for JSRR instruction\n");
                    EXIT(ERR_MALFORMED_INSTR);
                }
                const Register base_reg = (instr >> 6) & BITS_LOW_3;
                const Word base = registers.general_purpose[base_reg];
                /* printf("JSRR: R%x = 0x%04x\n", base_reg, base); */
                registers.program_counter = base;
            }
        }; break;

        // LD*
        case OPCODE_LD: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const SignedWord offset = low_9_bits_signed(instr);
            const Word value = memory[registers.program_counter + offset];
            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
            /* dbg_print_registers(); */
        }; break;

        // ST
        case OPCODE_ST: {
            /* printf("STORE\n"); */
            const Register src_reg = (instr >> 9) & BITS_LOW_3;
            const SignedWord offset = low_9_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];
            memory[registers.program_counter + offset] = value;
        }; break;

        // LDR*
        case OPCODE_LDR: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const Register base_reg = (instr >> 6) & BITS_LOW_3;
            const SignedWord offset = low_6_bits_signed(instr);
            const Word base = registers.general_purpose[base_reg];
            const Word value = memory[base + offset];
            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STR
        case OPCODE_STR: {
            const Register src_reg = (instr >> 9) & BITS_LOW_3;
            const Register base_reg = (instr >> 6) & BITS_LOW_3;
            const Word offset_bits = instr & BITS_LOW_6;
            const SignedWord offset = low_6_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];
            const Word base = registers.general_purpose[base_reg];
            memory[base + offset] = value;
        }; break;

        // LDI+
        case OPCODE_LDI: {
            const Register dest_reg = (instr >> 9) & BITS_LOW_3;
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = memory[registers.program_counter + offset];
            const Word value = memory[pointer];
            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STI
        case OPCODE_STI: {
            const Register src_reg = (instr >> 9) & BITS_LOW_3;
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = registers.general_purpose[src_reg];
            const Word value = memory[pointer];
            memory[registers.program_counter + offset] = value;
        }; break;

        // LEA*
        case OPCODE_LEA: {
            const Register dest_reg = (instr >> 9) & 0b111;
            const SignedWord offset = low_9_bits_signed(instr);
            /* printf(">LEA REG%d, pc_offset:0x%04hx\n", reg, */
            /*        pc_offset); */
            /* print_registers(); */
            registers.general_purpose[dest_reg] =
                registers.program_counter + offset;
            /* dbg_print_registers(); */
        }; break;

        // TRAP
        case OPCODE_TRAP:
            if (execute_trap_instruction(instr)) {
                return true;
            }
            break;

        // RTI (supervisor-only)
        case OPCODE_RTI:
            fprintf(stderr,
                    "Invalid use of RTI opcode: 0b%s in non-supervisor mode\n",
                    binary_string_word(OPCODE_RTI));
            EXIT(ERR_MALFORMED_INSTR);
            break;

        // (reserved)
        case OPCODE_RESERVED:
            fprintf(stderr, "Invalid reserved opcode: 0b%s\n",
                    binary_string_word(OPCODE_RESERVED));
            EXIT(ERR_MALFORMED_INSTR);
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0b%s (0x%04x)\n",
                    binary_string_word(opcode), opcode);
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
        static_cast<enum TrapVector>(instr & BITS_LOW_8);

    switch (trap_vector) {
        case TRAP_GETC: {
            tty_nobuffer_noecho();                      // Disable echo
            const char input = getchar() & BITS_LOW_8;  // Zero high 8 bits
            tty_restore();
            registers.general_purpose[0] = input;
            /* dbg_print_registers(); */
        }; break;

        case TRAP_IN: {
            printf("Input a character> ");
            tty_nobuffer_yesecho();                     // Enable echo
            const char input = getchar() & BITS_LOW_8;  // Zero high 8 bits
            tty_restore();
            registers.general_purpose[0] = input;
        }; break;

        case TRAP_OUT: {
            const Word word = registers.general_purpose[0];
            const char ch = static_cast<char>(word);
            print_char(ch);
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
                print_char(ch);
            }
        } break;

        case TRAP_PUTSP: {
            const Word *const str_words = &memory[registers.general_purpose[0]];
            const char *str = reinterpret_cast<const char *>(str_words);
            for (Word ch; (ch = str[0]) != 0x00; ++str) {
                print_char(ch);
            }
        }; break;

        case TRAP_HALT:
            return true;
            break;

        default:
            fprintf(stderr, "Invalid trap vector 0x%02x\n", trap_vector);
            EXIT(ERR_MALFORMED_INSTR);
    }

    return false;
}
