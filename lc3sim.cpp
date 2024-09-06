#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ERR_FILE 1
#define ERR_MALFORMED_INSTR 2
#define ERR_UNIMPLEMENTED 3

#define MEMORY_SIZE 0x10000L  // Total amount of allocated WORDS in memory
#define GP_REGISTER_COUNT 8   // Amount of general purpose registers

#define BITS_LOW_3 0b111
#define BITS_LOW_5 0x1f
#define BITS_LOW_8 0x00ff
#define BITS_LOW_9 0x01ff
#define BITS_HIGH_9 0xff80

#define WORD_SIZE sizeof(Word)

// For `ADD` and `AND` instructions
#define ARITH_IS_IMMEDIATE(instr) (bool)(((instr) >> 5) && 0b1)

#define UNIMPLEMENTED_INSTR(instr, name) \
    { fprintf(stderr, "Unimplemented instruction: 0b%04x: %s\n", instr, name); }
#define UNIMPLEMENTED_TRAP(vector, name)                                   \
    {                                                                      \
        fprintf(stderr, "Unimplemented trap vector: 0x%02x: %s\n", vector, \
                name);                                                     \
    }

typedef uint16_t Word;

typedef uint8_t RegisterCode;  // 3 bits
typedef uint8_t Immediate5;    // 5 bits
typedef uint16_t Offset9;      // 9 bits

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
} Registers;

// Exists for program lifetime, but must still be deleted before exit
// Dynamically allocated due to large size
static Word *memory = new Word[MEMORY_SIZE];

static Registers registers;

void free_memory() {
    delete[] memory;
    memory = nullptr;
}

Word swap_endianess(Word word) { return (word << 8) | (word >> 8); }

void read_file_to_memory(const char *filename, Word &start, Word &end) {
    FILE *file = fopen(filename, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(ERR_FILE);
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
    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        Word value = registers.general_purpose[reg];
        printf("    R%d  0x%04hx  %3d\n", reg, value, value);
    }
    printf("--------------------------\n");
}

int main() {
    const char *filename = "example/example.obj";

    Word file_start;
    Word file_end;
    read_file_to_memory(filename, file_start, file_end);

    /* for (size_t i = file_start - 2; i < file_end + 2; ++i) { */
    /*     printf("FILE: 0x%04lx: 0x%04hx\n", i, memory[i]); */
    /* } */

    // GP registers are already initialized to 0
    registers.program_counter = file_start;
    registers.stack_pointer = file_end;
    registers.frame_pointer = file_end;

    while (true) {
        Word instr = memory[registers.program_counter];
        ++registers.program_counter;

        /* printf("INSTR: 0x%04x  %16b\n", instr, instr); */

        // May be invalid enum variant
        // Handled in default switch branch
        Opcode opcode = static_cast<Opcode>(instr >> 12);

        switch (opcode) {
            // ADD+
            case OPCODE_ADD: {
                RegisterCode dest_reg = (instr >> 9) & BITS_LOW_3;
                RegisterCode src_reg1 = (instr >> 6) & BITS_LOW_3;
                Word value;
                bool is_immediate = ARITH_IS_IMMEDIATE(instr);
                if (!is_immediate) {
                    // 2 bits padding
                    uint8_t padding = (instr >> 3) & (0b11);
                    if (padding != 0b00) {
                        fprintf(stderr,
                                "Expected padding 0x00 for ADD instruction "
                                "0b0001\n");
                        free_memory();
                        exit(ERR_MALFORMED_INSTR);
                    }
                    RegisterCode src_reg2 = instr & BITS_LOW_3;
                    value = memory[registers.general_purpose[src_reg2]];
                } else {
                    Immediate5 imm = instr & BITS_LOW_5;
                    value = static_cast<Word>(imm);
                }
                printf(">ADD R%d = R%d + 0x%04hx\n", dest_reg, src_reg1, value);
                registers.general_purpose[dest_reg] =
                    registers.general_purpose[src_reg1] + value;
                dbg_print_registers();
                // TODO: Update condition codes
            }; break;

            // AND+
            case OPCODE_AND: {
                UNIMPLEMENTED_INSTR(instr, "AND");
            }; break;

            // BR
            case OPCODE_BR: {
                UNIMPLEMENTED_INSTR(instr, "BR");
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

            // LDI+
            case OPCODE_LDI: {
                UNIMPLEMENTED_INSTR(instr, "LDI");
            }; break;

            // LDR+
            case OPCODE_LDR: {
                UNIMPLEMENTED_INSTR(instr, "LDR");
            }; break;

            // LEA+
            case OPCODE_LEA: {
                RegisterCode dest_reg = (instr >> 9) & 0b111;
                Offset9 pc_offset = instr & BITS_LOW_9;
                /* printf(">LEA REG%d, pc_offset:0x%04hx\n", reg, */
                /*        pc_offset); */
                /* print_registers(); */
                registers.general_purpose[dest_reg] =
                    registers.program_counter + pc_offset;
                dbg_print_registers();
            }; break;

            // NOT+
            case OPCODE_NOT: {
                UNIMPLEMENTED_INSTR(instr, "NOT");
            }; break;

            // ST
            case OPCODE_ST: {
                UNIMPLEMENTED_INSTR(instr, "ST");
            }; break;

            // STI
            case OPCODE_STI: {
                UNIMPLEMENTED_INSTR(instr, "STI");
            }; break;

            // STR
            case OPCODE_STR: {
                UNIMPLEMENTED_INSTR(instr, "STR");
            }; break;

            // TRAP
            case OPCODE_TRAP: {
                // 4 bits padding
                uint8_t padding = (instr >> 8) & (0x0f);
                if (padding != 0x0) {
                    fprintf(
                        stderr,
                        "Expected padding 0x00 for TRAP instruction 0b1111\n");
                    free_memory();
                    exit(ERR_MALFORMED_INSTR);
                }

                // May be invalid enum variant
                // Handled in default switch branch
                enum TrapVector trap_vector =
                    static_cast<enum TrapVector>(instr & BITS_LOW_9);

                switch (trap_vector) {
                    case TRAP_GETC: {
                        UNIMPLEMENTED_TRAP(trap_vector, "GETC");
                    }; break;

                    case TRAP_OUT: {
                        UNIMPLEMENTED_TRAP(trap_vector, "OUT");
                    }; break;

                    case TRAP_PUTS: {
                        Word *str = &memory[registers.general_purpose[0]];
                        for (Word ch; (ch = str[0]) != 0x0000; ++str) {
                            if (ch & BITS_HIGH_9) {
                                fprintf(stderr,
                                        "String contains non-ASCII characters, "
                                        "which are not supported.");
                                free_memory();
                                exit(ERR_UNIMPLEMENTED);
                            }
                            printf("%c", ch);
                        }
                    } break;

                    case TRAP_IN: {
                        UNIMPLEMENTED_TRAP(trap_vector, "IN");
                    }; break;

                    case TRAP_PUTSP: {
                        UNIMPLEMENTED_TRAP(trap_vector, "PUTSP");
                    }; break;

                    case TRAP_HALT: {
                        goto end_program;
                    }; break;

                    default:
                        fprintf(stderr, "Invalid trap vector 0x%02x\n",
                                trap_vector);
                        free_memory();
                        exit(ERR_MALFORMED_INSTR);
                }
            } break;

            // (reserved)
            case OPCODE_RESERVED:
                fprintf(stderr, "Invalid reserved opcode: 0x%04x\n", opcode);
                free_memory();
                exit(ERR_MALFORMED_INSTR);
                break;

            // Invalid enum variant
            default:
                fprintf(stderr, "Invalid opcode: 0x%04x\n", opcode);
                free_memory();
                exit(ERR_MALFORMED_INSTR);
        }
    }
end_program:

    free_memory();

    return 0;
}
