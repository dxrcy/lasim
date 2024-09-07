#ifndef EXECUTE_CPP
#define EXECUTE_CPP

#include <cstdio>  // printf, fprintf

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

Error execute(void);
Error execute_next_instrution(bool &do_halt);
Error execute_trap_instruction(const Word instr, bool &do_halt);
void _dbg_print_registers(void);
Word &memory_get(Word addr);
SignedWord sign_extend(SignedWord value, const size_t size);
void set_condition_codes(const Word result);
Error print_char(const char ch);
void print_on_new_line(void);
static char *halfbyte_string(const Word word);

Error execute() {
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
    const Word instr = memory_get(registers.program_counter);
    ++registers.program_counter;

    // printf("INSTR at 0x%04x: 0x%04x  %016b\n", registers.program_counter - 1,
    //        instr, instr);

    if (instr == 0xdddd) {
        fprintf(stderr,
                "DEBUG: Attempt to execute sentinal word 0xdddd."
                " This is probably a bug\n");
        exit(ERR_BAD_ADDRESS);
    }
    if (instr == 0xeeee) {
        fprintf(stderr,
                "DEBUG: Attempt to execute sentinal word 0xeeee."
                " This is probably a bug\n");
        exit(ERR_BAD_ADDRESS);
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
                    exit(ERR_MALFORMED_PADDING);
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
                    exit(ERR_MALFORMED_PADDING);
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
                exit(ERR_MALFORMED_PADDING);
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
                exit(ERR_MALFORMED_PADDING);
            }
            // 6 bits padding
            // After base register
            const uint8_t padding_2 = bits_0_6(instr);
            if (padding_2 != 0b000000) {
                fprintf(stderr,
                        "Expected padding 0b000000 for JMP/RET instruction\n");
                exit(ERR_MALFORMED_PADDING);
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
                    exit(ERR_MALFORMED_PADDING);
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
            const Word value = memory_get(registers.program_counter + offset);
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
            memory_get(registers.program_counter + offset) = value;
        }; break;

        // LDR*
        case OPCODE_LDR: {
            const Register dest_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_signed(instr);
            const Word base = registers.general_purpose[base_reg];
            const Word value = memory_get(base + offset);
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
            memory_get(base + offset) = value;
        }; break;

        // LDI+
        case OPCODE_LDI: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = memory_get(registers.program_counter + offset);
            const Word value = memory_get(pointer);
            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STI
        case OPCODE_STI: {
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = registers.general_purpose[src_reg];
            const Word value = memory_get(pointer);
            memory_get(registers.program_counter + offset) = value;
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
            exit(ERR_UNAUTHORIZED_INSTR);
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0b%s (0x%04x)\n",
                    halfbyte_string(opcode), opcode);
            exit(ERR_MALFORMED_INSTR);
    }

    return ERR_OK;
}

Error execute_trap_instruction(const Word instr, bool &do_halt) {
    // 4 bits padding
    const uint8_t padding = bits_8_12(instr);
    if (padding != 0b0000) {
        fprintf(stderr, "Expected padding 0x00 for TRAP instruction\n");
        exit(ERR_MALFORMED_PADDING);
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
            IGNORE_ERR(print_char(input));
            if (!stdout_on_new_line) {
                printf("\n");
            }
            registers.general_purpose[0] = input;
        }; break;

        case TRAP_OUT: {
            const Word word = registers.general_purpose[0];
            const char ch = static_cast<char>(word);
            RETURN_IF_ERR(print_char(ch));
        }; break;

        case TRAP_PUTS: {
            print_on_new_line();
            for (Word i = registers.general_purpose[0];; ++i) {
                const Word ch = memory_get(i);
                if (ch == 0x0000) break;
                RETURN_IF_ERR(print_char(ch));
            }
        } break;

        case TRAP_PUTSP: {
            print_on_new_line();
            for (Word i = registers.general_purpose[0];; ++i) {
                const Word word = memory_get(i);
                const uint8_t high = bits_high(word);
                const uint8_t low = bits_low(word);
                if (high == 0x0000) break;
                RETURN_IF_ERR(print_char(high));
                if (low == 0x0000) break;
                RETURN_IF_ERR(print_char(low));
            }
        }; break;

        case TRAP_HALT:
            do_halt = true;
            return ERR_OK;
            break;

        default:
            fprintf(stderr, "Invalid trap vector 0x%02x\n", trap_vector);
            exit(ERR_MALFORMED_TRAP);
    }

    return ERR_OK;
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

// TODO: Return Error
// Check memory address, then return reference to memory value
Word &memory_get(Word addr) {
    if (addr < memory_file_bounds.start) {
        fprintf(stderr, "Cannot access non-user memory (before origin)\n");
        exit(ERR_BAD_ADDRESS);
    }
    if (addr >= memory_file_bounds.end) {
        fprintf(stderr, "Cannot access non-file memory (after end of file)\n");
        exit(ERR_BAD_ADDRESS);
    }
    return memory[addr];
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

Error print_char(const char ch) {
    if (ch == '\r') {
        printf("\n");
    } else {
        printf("%c", ch);
    }
    // If a bit not between [0,7] is set
    if (ch & ~BITMASK_LOW_7) {
        fprintf(stderr,
                "String contains non-ASCII characters, "
                "which are not supported.");
        exit(ERR_UNIMPLEMENTED);
    }
    stdout_on_new_line = ch == '\n' || ch == '\r';
    return ERR_OK;
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

#endif
