#include <cstdint>
#include <cstring>
#include <fstream>

using std::ifstream;

#define MEMORY_SIZE 0x10000L
#define GP_REGISTER_COUNT 8  // Amount of general purpose registers

#define BITS_LOW_9 0x01ff
#define BITS_LOW_8 0x00ff
#define BITS_HIGH_9 0xff80

#define ERR_FILE 1
#define ERR_MALFORMED_INSTR 2
#define ERR_UNIMPLEMENTED 3

typedef uint16_t Word;
typedef uint8_t Opcode;        // 4 bits
typedef uint8_t RegisterCode;  // 3 bits
typedef uint16_t Offset9;      // 9 bits

static Word memory[MEMORY_SIZE];

static Word registers[GP_REGISTER_COUNT] = {0};
static Word program_counter;
static Word stack_pointer;
static Word frame_pointer;

Word swap_endianess(Word word) { return (word << 8) | (word >> 8); }

void read_file_to_memory(const char *filename, Word &start, Word &end) {
    ifstream file(filename);
    if (!file.is_open()) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(ERR_FILE);
    }

    // TODO: Handle failure
    file.read(reinterpret_cast<char *>(&start), sizeof(Word));

    /* printf("origin: 0x%04x\n", start); */

    // TODO: Handle failure
    file.read(reinterpret_cast<char *>(memory + start), sizeof(memory));
    std::streamsize words_read = file.gcount() / 2;

    end = start + words_read;

    // Mark undefined bytes for debugging
    memset(memory, 0xdd, start * 2);                       // Before file
    memset(memory + end, 0xee, sizeof(memory) - end * 2);  // After file

    // TODO: Make this better !!
    for (size_t i = start; i < end; ++i) {
        memory[i] = swap_endianess(memory[i]);
    }

    /* printf("bytes read: %ld\n", words_read); */
}

void print_registers() {
    printf("--------------------------\n");
    printf("    PC  0x%04hx\n", program_counter);
    printf("    SP  0x%04hx\n", stack_pointer);
    printf("    FP  0x%04hx\n", frame_pointer);
    printf("..........................\n");
    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        Word value = registers[reg];
        printf("    R%d  0x%04hx  %3d\n", reg, value, value);
    }
    printf("--------------------------\n");
}

int main() {
    const char *filename = "example/example.obj";

    Word file_start;
    Word file_end;
    read_file_to_memory(filename, file_start, file_end);

    /* for (size_t i = file_start; i < file_end; ++i) { */
    /*     printf("FILE: 0x%04lx: 0x%04hx\n", i, memory[i]); */
    /* } */

    program_counter = file_start;
    stack_pointer = file_end;
    frame_pointer = file_end;

    while (true) {
        Word instr = memory[program_counter];
        ++program_counter;

        /* printf("INSTR: 0x%04x  %16b\n", instr, instr); */

        Opcode opcode = instr >> 12;

        switch (opcode) {
            // LEA+
            case 0b1110: {
                RegisterCode reg = (instr >> 9) & 0b111;
                Offset9 pc_offset = instr & BITS_LOW_9;
                /* printf(">LEA reg:0x%04hhx, pc_offset:0x%04hx\n", reg, */
                /*        pc_offset); */
                /* print_registers(); */
                registers[reg] = program_counter + pc_offset;
                /* print_registers(); */
            }; break;

            // TRAP
            case 0b1111: {
                // 4 bits padding
                uint8_t padding = (instr >> 8) & (0x0f);
                if (padding != 0x00) {
                    fprintf(
                        stderr,
                        "Expected padding 0x00 for TRAP instruction 0b1111\n");
                    exit(ERR_MALFORMED_INSTR);
                }
                uint8_t trap_vector = instr & BITS_LOW_9;

                switch (trap_vector) {
                    // PUTS
                    case 0x22: {
                        Word *str = &memory[registers[0]];
                        for (Word ch; (ch = str[0]) != 0x0000; ++str) {
                            if (ch & BITS_HIGH_9) {
                                fprintf(stderr,
                                        "String contains non-ASCII characters, "
                                        "which are not supported.");
                                exit(ERR_UNIMPLEMENTED);
                            }
                            printf("%c", ch);
                        }
                    } break;

                    // HALT
                    case 0x25: {
                        goto end_program;
                    }; break;

                    default:
                        fprintf(stderr, "Invalid trap vector 0x%02x\n",
                                trap_vector);
                        exit(ERR_MALFORMED_INSTR);
                }
            } break;

            default:
                fprintf(stderr, "Invalid opcode: 0x%04x\n", opcode);
                exit(ERR_MALFORMED_INSTR);
        }
    }
end_program:;

    return 0;
}
