#ifndef EXECUTE_CPP
#define EXECUTE_CPP

#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memset

#include "error.hpp"
#include "globals.cpp"
#include "tty.cpp"
#include "types.hpp"

#define BITMASK_LOW_2 0b0000'0000'0000'0011
#define BITMASK_LOW_3 0b0000'0000'0000'0111
#define BITMASK_LOW_4 0b0000'0000'0000'1111
#define BITMASK_LOW_5 0b0000'0000'0001'1111
#define BITMASK_LOW_6 0b0000'0000'0011'1111
#define BITMASK_LOW_7 0b0000'0000'0111'1111
#define BITMASK_LOW_8 0b0000'0000'1111'1111
#define BITMASK_LOW_9 0b0000'0001'1111'1111
#define BITMASK_LOW_11 0b0000'0111'1111'1111

#define bit_5(instr) (((instr) >> 5) & 1)
#define bit_11(instr) (((instr) >> 11) & 1)
#define bit_15(instr) (((instr) >> 15) & 1)

#define bits_0_2(word) ((word) & BITMASK_LOW_3)
#define bits_0_5(word) ((word) & BITMASK_LOW_5)
#define bits_0_6(word) ((word) & BITMASK_LOW_6)
#define bits_0_8(word) ((word) & BITMASK_LOW_8)
#define bits_3_4(word) (((word) >> 3) & BITMASK_LOW_2)
#define bits_6_8(word) (((word) >> 6) & BITMASK_LOW_3)
#define bits_8_12(word) (((word) >> 8) & BITMASK_LOW_4)
#define bits_9_10(word) (((word) >> 9) & BITMASK_LOW_2)
#define bits_9_11(word) (((word) >> 9) & BITMASK_LOW_3)
#define bits_12_15(word) (((word) >> 12) & BITMASK_LOW_4)

#define bits_high(word) (((word) >> 8) & BITMASK_LOW_8)
#define bits_low(word) ((word) & BITMASK_LOW_8)

#define to_signed_word(value, size) \
    (sign_extend(static_cast<SignedWord>(value), size))

#define low_6_bits_signed(instr) (to_signed_word((instr) & BITMASK_LOW_6, 6))
#define low_9_bits_signed(instr) (to_signed_word((instr) & BITMASK_LOW_9, 9))
#define low_11_bits_signed(instr) (to_signed_word((instr) & BITMASK_LOW_11, 11))

// Swap high and low bytes of a word
#define swap_endianess(word) (((word) << 8) | ((word) >> 8))

#define MEMORY_CHECK_RETURN_ERR(addr) RETURN_IF_ERR(memory_check(addr))

// Check memory address is within the 'allocated' file memory
Error memory_check(Word addr) {
    if (addr < memory_file_bounds.start) {
        fprintf(stderr, "Cannot access non-user memory (before user memory)\n");
        return ERR_ADDRESS_TOO_LOW;
    }
    if (addr > MEMORY_USER_MAX) {
        fprintf(stderr, "Cannot access non-user memory (after user memory)\n");
        return ERR_ADDRESS_TOO_HIGH;
    }
    return ERR_OK;
}

Error execute(const char *const obj_file);
Error execute_next_instrution(bool &do_halt);
Error execute_trap_instruction(const Word instr, bool &do_halt);

Error read_obj_file_to_memory(const char *const obj_file);

SignedWord sign_extend(SignedWord value, const size_t size);
void set_condition_codes(const Word result);
void print_char(const char ch);
void print_on_new_line(void);
static char *halfbyte_string(const Word word);

void _dbg_print_registers(void);

Error execute(const char *const obj_file) {
    // TODO: Allocate `memory` here

    RETURN_IF_ERR(read_obj_file_to_memory(obj_file));

    // GP and condition registers are already initialized to 0
    registers.program_counter = memory_file_bounds.start;
    registers.stack_pointer = memory_file_bounds.end;
    registers.frame_pointer = memory_file_bounds.end;

    // Loop until `true` is returned, indicating a HALT (TRAP 0x25)
    bool do_halt = false;
    while (!do_halt) {
        RETURN_IF_ERR(execute_next_instrution(do_halt));
    }

    if (!stdout_on_new_line) {
        printf("\n");
    }

    free_memory();
    return ERR_OK;
}

// `true` return value indicates that program should end
Error execute_next_instrution(bool &do_halt) {
    MEMORY_CHECK_RETURN_ERR(registers.program_counter);
    const Word instr = memory[registers.program_counter];
    ++registers.program_counter;

    // printf("INSTR at 0x%04x: 0x%04x  %016b\n", registers.program_counter - 1,
    //        instr, instr);

    // TODO: Maybe only run in debug mode ?
    if (instr == 0xdddd) {
        fprintf(stderr,
                "DEBUG: Attempt to execute sentinal word 0xdddd."
                " This is probably a bug\n");
        return ERR_ADDRESS_TOO_LOW;
    }
    if (instr == 0xeeee) {
        fprintf(stderr,
                "DEBUG: Attempt to execute sentinal word 0xeeee."
                " This is probably a bug\n");
        return ERR_ADDRESS_TOO_HIGH;
    }

    // May be invalid enum variant
    // Handled in default switch branch
    const Opcode opcode = static_cast<Opcode>(bits_12_15(instr));

    // TODO: Check all operands for whether they need to be sign-extended !!!!

    switch (opcode) {
        // ADD*
        case OPCODE_ADD: {
            const Register dest_reg = bits_9_11(instr);
            const Register src_reg_a = bits_6_8(instr);

            const SignedWord value_a =
                static_cast<SignedWord>(registers.general_purpose[src_reg_a]);
            SignedWord value_b;

            if (bit_5(instr) == 0b0) {
                // 2 bits padding
                const uint8_t padding = bits_3_4(instr);
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for ADD instruction\n");
                    return ERR_MALFORMED_PADDING;
                }
                const Register src_reg_b = bits_0_2(instr);
                value_b = static_cast<SignedWord>(
                    registers.general_purpose[src_reg_b]);
            } else {
                value_b = to_signed_word(bits_0_5(instr), 5);
            }

            const Word result = static_cast<Word>(value_a + value_b);

            // printf("\n>ADD R%d = (R%d) 0x%04hx + 0x%04hx = 0x%04hx\n",
            // dest_reg, src_reg_a, value_a, value_b, result);

            registers.general_purpose[dest_reg] = result;

            /* _dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // AND*
        case OPCODE_AND: {
            const Register dest_reg = bits_9_11(instr);
            const Register src_reg_a = bits_6_8(instr);

            const Word value_a = registers.general_purpose[src_reg_a];
            Word value_b;

            if (bit_5(instr) == 0b0) {
                // 2 bits padding
                const uint8_t padding = bits_3_4(instr);
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for AND instruction\n");
                    return ERR_MALFORMED_PADDING;
                }
                const Register src_reg_b = bits_0_2(instr);
                value_b = registers.general_purpose[src_reg_b];
            } else {
                value_b = static_cast<SignedWord>(bits_0_5(instr));
            }

            const Word result = value_a & value_b;

            /* printf(">AND R%d = (R%d) 0x%04hx & 0x%04hx = 0x%04hx\n",
             * dest_reg, */
            /*        src_reg_a, value_a, value_b, result); */

            registers.general_purpose[dest_reg] = result;

            /* dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // NOT*
        case OPCODE_NOT: {
            const Register dest_reg = bits_9_11(instr);
            const Register src_reg = bits_6_8(instr);

            // 4 bits ONEs padding
            const uint8_t padding = bits_0_5(instr);
            if (padding != BITMASK_LOW_5) {
                fprintf(stderr,
                        "Expected padding 0x11111 for NOT instruction\n");
                return ERR_MALFORMED_PADDING;
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
            const ConditionCode condition = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);

            /* printf("BR: %03b & %03b = %03b -> %b\n", condition, */
            /*        registers.condition, condition & registers.condition, */
            /*        (condition & registers.condition) != 0b000); */
            /* printf("PCOffset: 0x%04x  %d\n", offset, offset); */
            /*  */
            /* _dbg_print_registers(); */

            // If any bits of the condition codes match
            if ((condition & registers.condition) != 0b000) {
                registers.program_counter += offset;
                /* printf("branched to 0x%04x\n", registers.program_counter); */
            }
        }; break;

        // JMP/RET
        case OPCODE_JMP_RET: {
            // 3 bits padding
            const uint8_t padding_1 = bits_9_11(instr);
            if (padding_1 != 0b000) {
                fprintf(stderr,
                        "Expected padding 0b000 for JMP/RET instruction\n");
                return ERR_MALFORMED_PADDING;
            }
            // 6 bits padding
            // After base register
            const uint8_t padding_2 = bits_0_6(instr);
            if (padding_2 != 0b000000) {
                fprintf(stderr,
                        "Expected padding 0b000000 for JMP/RET instruction\n");
                return ERR_MALFORMED_PADDING;
            }
            const Register base_reg = bits_6_8(instr);
            const Word base = registers.general_purpose[base_reg];
            registers.program_counter = base;
        }; break;

        // JSR/JSRR
        case OPCODE_JSR_JSRR: {
            // Save PC to R7
            registers.general_purpose[7] = registers.program_counter;
            // Bit 11 defines JSR or JSRR
            const bool is_offset = bit_11(instr) == 0b1;
            if (is_offset) {
                // JSR
                const SignedWord offset = low_11_bits_signed(instr);
                /* printf("JSR: PCOffset = 0x%04x\n", pc_offset); */
                registers.program_counter += offset;
            } else {
                // JSRR
                // 2 bits padding
                const uint8_t padding = bits_9_10(instr);
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for JSRR instruction\n");
                    return ERR_MALFORMED_PADDING;
                }
                const Register base_reg = bits_6_8(instr);
                const Word base = registers.general_purpose[base_reg];
                /* printf("JSRR: R%x = 0x%04x\n", base_reg, base); */
                registers.program_counter = base;
            }
        }; break;

        // LD*
        case OPCODE_LD: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);

            MEMORY_CHECK_RETURN_ERR(registers.program_counter + offset);
            const Word value = memory[registers.program_counter + offset];

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
            /* dbg_print_registers(); */
        }; break;

        // ST
        case OPCODE_ST: {
            /* printf("STORE\n"); */
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];

            MEMORY_CHECK_RETURN_ERR(registers.program_counter + offset);
            memory[registers.program_counter + offset] = value;
        }; break;

        // LDR*
        case OPCODE_LDR: {
            const Register dest_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_signed(instr);
            const Word base = registers.general_purpose[base_reg];

            MEMORY_CHECK_RETURN_ERR(base + offset);
            const Word value = memory[base + offset];

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STR
        case OPCODE_STR: {
            const Register src_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];
            const Word base = registers.general_purpose[base_reg];

            MEMORY_CHECK_RETURN_ERR(base + offset);
            memory[base + offset] = value;
        }; break;

        // LDI+
        case OPCODE_LDI: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);

            MEMORY_CHECK_RETURN_ERR(registers.program_counter + offset);
            const Word pointer = memory[registers.program_counter + offset];
            MEMORY_CHECK_RETURN_ERR(pointer);
            const Word value = memory[pointer];

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STI
        case OPCODE_STI: {
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = registers.general_purpose[src_reg];

            MEMORY_CHECK_RETURN_ERR(pointer);
            MEMORY_CHECK_RETURN_ERR(registers.program_counter + offset);
            const Word value = memory[pointer];
            memory[registers.program_counter + offset] = value;
        }; break;

        // LEA*
        case OPCODE_LEA: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            /* printf(">LEA REG%d, pc_offset:0x%04hx\n", reg, */
            /*        pc_offset); */
            /* print_registers(); */
            registers.general_purpose[dest_reg] =
                registers.program_counter + offset;
            /* dbg_print_registers(); */
        }; break;

        // TRAP
        case OPCODE_TRAP: {
            RETURN_IF_ERR(execute_trap_instruction(instr, do_halt));
        }; break;

        // RTI (supervisor-only)
        case OPCODE_RTI:
            fprintf(stderr,
                    "Invalid use of RTI opcode: 0b%s in non-supervisor mode\n",
                    halfbyte_string(OPCODE_RTI));
            return ERR_UNAUTHORIZED_INSTR;
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0b%s (0x%04x)\n",
                    halfbyte_string(opcode), opcode);
            return ERR_MALFORMED_INSTR;
    }

    return ERR_OK;
}

#define IS_ASCII_OR_RETURN_ERR(word)                         \
    {                                                        \
        if (word & ~BITMASK_LOW_7) {                         \
            fprintf(stderr,                                  \
                    "String contains non-ASCII characters, " \
                    "which are not supported.");             \
            return ERR_UNIMPLEMENTED;                        \
        }                                                    \
    }

Error execute_trap_instruction(const Word instr, bool &do_halt) {
    // 4 bits padding
    const uint8_t padding = bits_8_12(instr);
    if (padding != 0b0000) {
        fprintf(stderr, "Expected padding 0x00 for TRAP instruction\n");
        return ERR_MALFORMED_PADDING;
    }

    // May be invalid enum variant
    // Handled in default switch branch
    const enum TrapVector trap_vector =
        static_cast<enum TrapVector>(bits_0_8(instr));

    switch (trap_vector) {
        case TRAP_GETC: {
            tty_nobuffer_noecho();                         // Disable echo
            const char input = getchar() & BITMASK_LOW_8;  // Zero high 8 bits
            tty_restore();
            registers.general_purpose[0] = input;
            /* dbg_print_registers(); */
        }; break;

        case TRAP_IN: {
            print_on_new_line();
            printf("Input a character> ");
            tty_nobuffer_noecho();
            const char input = getchar() & BITMASK_LOW_8;  // Zero high 8 bits
            tty_restore();
            // Echo, follow with newline if not already printed
            // Don't check if input is ASCII, it doesn't matter
            print_char(input);
            if (!stdout_on_new_line) {
                printf("\n");
            }
            registers.general_purpose[0] = input;
        }; break;

        case TRAP_OUT: {
            const Word word = registers.general_purpose[0];
            IS_ASCII_OR_RETURN_ERR(word);
            print_char(static_cast<char>(word));
        }; break;

        case TRAP_PUTS: {
            print_on_new_line();
            for (Word i = registers.general_purpose[0];; ++i) {
                MEMORY_CHECK_RETURN_ERR(i);
                const Word word = memory[i];
                if (word == 0x0000) break;
                IS_ASCII_OR_RETURN_ERR(word);
                print_char(static_cast<char>(word));
            }
        } break;

        case TRAP_PUTSP: {
            print_on_new_line();
            // Loop over words, then split into bytes
            // This is done to ensure the memory check is sound
            for (Word i = registers.general_purpose[0];; ++i) {
                MEMORY_CHECK_RETURN_ERR(i);
                const Word word = memory[i];
                const uint8_t high = bits_high(word);
                const uint8_t low = bits_low(word);
                if (high == 0x0000) break;
                IS_ASCII_OR_RETURN_ERR(high);
                print_char(high);
                if (low == 0x0000) break;
                IS_ASCII_OR_RETURN_ERR(low);
                print_char(low);
            }
        }; break;

        case TRAP_HALT:
            do_halt = true;
            return ERR_OK;
            break;

        default:
            fprintf(stderr, "Invalid trap vector 0x%02x\n", trap_vector);
            return ERR_MALFORMED_TRAP;
    }

    return ERR_OK;
}

Error read_obj_file_to_memory(const char *const obj_file) {
    FILE *const file = fopen(obj_file, "rb");
    size_t words_read;

    if (file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", obj_file);
        return ERR_FILE_OPEN;
    }

    Word origin;
    words_read = fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, file);

    if (ferror(file)) {
        fprintf(stderr, "Could not read file %s\n", obj_file);
        return ERR_FILE_READ;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_file);
        return ERR_FILE_TOO_SHORT;
    }

    Word start = swap_endianess(origin);

    /* printf("origin: 0x%04x\n", start); */

    char *const memory_at_file = reinterpret_cast<char *>(memory + start);
    const size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    words_read = fread(memory_at_file, WORD_SIZE, max_file_bytes, file);

    if (ferror(file)) {
        fprintf(stderr, "Could not read file %s\n", obj_file);
        return ERR_FILE_READ;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_file);
        return ERR_FILE_TOO_SHORT;
    }
    if (!feof(file)) {
        fprintf(stderr, "File is too long %s\n", obj_file);
        return ERR_FILE_TOO_LONG;
    }

    Word end = start + words_read;

    // Mark undefined bytes for debugging
    memset(memory, 0xdd, start * WORD_SIZE);  // Before file
    memset(memory + end, 0xee,
           (MEMORY_SIZE - end) * WORD_SIZE);  // After file

    // TODO: Make this better !!
    // ^ Read file word-by-word, and swap endianess in the same loop
    for (size_t i = start; i < end; ++i) {
        memory[i] = swap_endianess(memory[i]);
    }

    /* printf("words read: %ld\n", words_read); */

    memory_file_bounds.start = start;
    memory_file_bounds.end = end;

    fclose(file);

    return ERR_OK;
}

SignedWord sign_extend(SignedWord value, const size_t size) {
    // If previous-highest bit is set
    if (value >> (size - 1) & 0b1) {
        // Set all bits higher than previous sign bit to 1
        return value | (~0U << size);
    }
    return value;
}

void set_condition_codes(const Word result) {
    const bool is_negative = bit_15(result) == 0b1;
    const bool is_zero = result == 0;
    const bool is_positive = !is_negative && !is_zero;
    // Set low 3 bits as N,Z,P
    registers.condition = (is_negative << 2) | (is_zero << 1) | is_positive;
}

void print_char(const char ch) {
    if (ch == '\r') {
        printf("\n");
    } else {
        printf("%c", ch);
    }
    stdout_on_new_line = ch == '\n' || ch == '\r';
}

void print_on_new_line() {
    if (!stdout_on_new_line) {
        printf("\n");
        stdout_on_new_line = true;
    }
}

// Since %b printf format specifier is not ISO-compliant
static char *halfbyte_string(const Word word) {
    static char str[5];
    for (int i = 0; i < 4; ++i) {
        str[i] = '0' + ((word >> (3 - i)) & 0b1);
    }
    str[4] = '\0';
    return str;
}

void _dbg_print_registers() {
    printf("--------------------------\n");
    printf("    PC  0x%04hx\n", registers.program_counter);
    printf("    SP  0x%04hx\n", registers.stack_pointer);
    printf("    FP  0x%04hx\n", registers.frame_pointer);
    printf("..........................\n");
    printf("    N=%x  Z=%x  P=%x\n",
           (registers.condition >> 2),        // Negative
           (registers.condition >> 1) & 0b1,  // Zero
           (registers.condition) & 0b1);      // Positive
    printf("..........................\n");
    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        const Word value = registers.general_purpose[reg];
        printf("    R%d  0x%04hx  %3d\n", reg, value, value);
    }
    printf("--------------------------\n");
}

#endif
