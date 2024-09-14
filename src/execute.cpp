#ifndef EXECUTE_CPP
#define EXECUTE_CPP

#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memset

#include "bitmasks.hpp"
#include "error.hpp"
#include "globals.hpp"
#include "tty.cpp"
#include "types.hpp"

#define to_signed_word(_value, _size) \
    (sign_extend(static_cast<SignedWord>(_value), (_size)))

#define low_6_bits_signed(_instr) (to_signed_word((_instr) & BITMASK_LOW_6, 6))
#define low_9_bits_signed(_instr) (to_signed_word((_instr) & BITMASK_LOW_9, 9))
#define low_11_bits_signed(_instr) \
    (to_signed_word((_instr) & BITMASK_LOW_11, 11))

// Prompt for `IN` trap
#define TRAP_IN_PROMPT "Input a character: "

// TODO(refactor): Re-order functions

void execute(const char *const obj_filename);
void execute_next_instrution(bool &do_halt);
void execute_trap_instruction(const Word instr, bool &do_halt);

void read_obj_filename_to_memory(const char *const obj_filename);

Word &memory_checked(Word addr);
SignedWord sign_extend(SignedWord value, const size_t size);
void set_condition_codes(const Word result);
void print_char(char ch);
void print_on_new_line(void);
static char *halfbyte_string(const Word word);
void print_registers(void);

void execute(const char *const obj_filename) {
    read_obj_filename_to_memory(obj_filename);
    OK_OR_RETURN();

    // GP and condition registers are already initialized to 0
    registers.program_counter = memory_file_bounds.start;

    // Loop until `true` is returned, indicating a HALT (TRAP 0x25)
    bool do_halt = false;
    while (!do_halt) {
        execute_next_instrution(do_halt);
        OK_OR_RETURN();
    }

    if (!stdout_on_new_line) {
        printf("\n");
    }
}

// `true` return value indicates that program should end
void execute_next_instrution(bool &do_halt) {
    memory_checked(registers.program_counter);
    OK_OR_RETURN();

    const Word instr = memory[registers.program_counter];
    ++registers.program_counter;

    // printf("INSTR at 0x%04x: 0x%04x  %016b\n", registers.program_counter - 1,
    //        instr, instr);

    // May be invalid enum variant
    // Handled in default switch branch
    const Opcode opcode = static_cast<Opcode>(bits_12_15(instr));

    // TODO(correctness): Check all operands for whether they need to be
    //     sign-extended !!!!

    switch (opcode) {
        // ADD*
        case Opcode::ADD: {
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
                    ERROR = ERR_MALFORMED_PADDING;
                    return;
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
        case Opcode::AND: {
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
                    ERROR = ERR_MALFORMED_PADDING;
                    return;
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
        case Opcode::NOT: {
            const Register dest_reg = bits_9_11(instr);
            const Register src_reg = bits_6_8(instr);

            // 4 bits ONEs padding
            const uint8_t padding = bits_0_5(instr);
            if (padding != BITMASK_LOW_5) {
                fprintf(stderr,
                        "Expected padding 0x11111 for NOT instruction\n");
                ERROR = ERR_MALFORMED_PADDING;
                return;
            }

            /* printf(">NOT R%d = NOT R%d\n", dest_reg, src_reg1); */

            const Word result = ~registers.general_purpose[src_reg];
            registers.general_purpose[dest_reg] = result;

            /* dbg_print_registers(); */

            set_condition_codes(result);
        }; break;

        // BRcc
        case Opcode::BR: {
            // Skip special NOP case
            if (instr == 0x0000) {
                break;
            }

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
            // Instruction `BR` is equivalent to `BRnzp`, but 0b000 is checked
            //     here for completeness (as if 0b111)
            if (condition == 0b000 ||
                (condition & registers.condition) != 0b000) {
                registers.program_counter += offset;
                /* printf("branched to 0x%04x\n", registers.program_counter); */
            }
        }; break;

        // JMP/RET
        case Opcode::JMP_RET: {
            // 3 bits padding
            const uint8_t padding_1 = bits_9_11(instr);
            if (padding_1 != 0b000) {
                fprintf(stderr,
                        "Expected padding 0b000 for JMP/RET instruction\n");
                ERROR = ERR_MALFORMED_PADDING;
                return;
            }
            // 6 bits padding
            // After base register
            const uint8_t padding_2 = bits_0_6(instr);
            if (padding_2 != 0b000000) {
                fprintf(stderr,
                        "Expected padding 0b000000 for JMP/RET instruction\n");
                ERROR = ERR_MALFORMED_PADDING;
                return;
            }
            const Register base_reg = bits_6_8(instr);
            const Word base = registers.general_purpose[base_reg];
            registers.program_counter = base;
        }; break;

        // JSR/JSRR
        case Opcode::JSR_JSRR: {
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
                    ERROR = ERR_MALFORMED_PADDING;
                    return;
                }
                const Register base_reg = bits_6_8(instr);
                const Word base = registers.general_purpose[base_reg];
                /* printf("JSRR: R%x = 0x%04x\n", base_reg, base); */
                registers.program_counter = base;
            }
        }; break;

        // LD*
        case Opcode::LD: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);

            const Word value =
                memory_checked(registers.program_counter + offset);
            OK_OR_RETURN();

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
            /* dbg_print_registers(); */
        }; break;

        // ST
        case Opcode::ST: {
            /* printf("STORE\n"); */
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];

            memory_checked(registers.program_counter + offset) = value;
            OK_OR_RETURN();
        }; break;

        // LDR*
        case Opcode::LDR: {
            const Register dest_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_signed(instr);
            const Word base = registers.general_purpose[base_reg];

            const Word value = memory_checked(base + offset);
            OK_OR_RETURN();

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STR
        case Opcode::STR: {
            const Register src_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_signed(instr);
            const Word value = registers.general_purpose[src_reg];
            const Word base = registers.general_purpose[base_reg];

            memory_checked(base + offset) = value;
            OK_OR_RETURN();
        }; break;

        // LDI+
        case Opcode::LDI: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);

            const Word pointer =
                memory_checked(registers.program_counter + offset);
            OK_OR_RETURN();
            const Word value = memory_checked(pointer);
            OK_OR_RETURN();

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STI
        case Opcode::STI: {
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_signed(instr);
            const Word pointer = registers.general_purpose[src_reg];

            const Word value = memory_checked(pointer);
            OK_OR_RETURN();
            memory_checked(registers.program_counter + offset) = value;
            OK_OR_RETURN();
        }; break;

        // LEA*
        case Opcode::LEA: {
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
        case Opcode::TRAP: {
            execute_trap_instruction(instr, do_halt);
            OK_OR_RETURN();
        }; break;

        // RTI (supervisor-only)
        case Opcode::RTI:
            fprintf(stderr,
                    "Invalid use of RTI opcode: 0b%s in non-supervisor mode\n",
                    halfbyte_string(static_cast<Word>(opcode)));
            ERROR = ERR_UNAUTHORIZED_INSTR;
            return;
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0b%s (0x%04x)\n",
                    halfbyte_string(static_cast<Word>(opcode)),
                    static_cast<Word>(opcode));
            ERROR = ERR_MALFORMED_INSTR;
            return;
    }
}

void execute_trap_instruction(const Word instr, bool &do_halt) {
    // 4 bits padding
    const uint8_t padding = bits_8_12(instr);
    if (padding != 0b0000) {
        fprintf(stderr, "Expected padding 0x00 for TRAP instruction\n");
        ERROR = ERR_MALFORMED_PADDING;
        return;
    }

    // May be invalid enum variant
    // Handled in default switch branch
    const TrapVector trap_vector = static_cast<TrapVector>(bits_0_8(instr));

    switch (trap_vector) {
        case TrapVector::GETC: {
            tty_nobuffer_noecho();                         // Disable echo
            const char input = getchar() & BITMASK_LOW_8;  // Zero high 8 bits
            tty_restore();
            registers.general_purpose[0] = input;
            /* dbg_print_registers(); */
        }; break;

        case TrapVector::IN: {
            print_on_new_line();
            printf(TRAP_IN_PROMPT);
            tty_nobuffer_noecho();
            const char input = getchar() & BITMASK_LOW_8;  // Zero high 8 bits
            tty_restore();
            print_char(input);
            registers.general_purpose[0] = input;
        }; break;

        case TrapVector::OUT: {
            const Word word = registers.general_purpose[0];
            const char ch = static_cast<char>(word & BITMASK_LOW_8);
            print_char(ch);
        }; break;

        case TrapVector::PUTS: {
            print_on_new_line();
            for (Word i = registers.general_purpose[0];; ++i) {
                const Word word = memory_checked(i);
                OK_OR_RETURN();

                if (word == 0x0000)
                    break;
                const char ch = static_cast<char>(word & BITMASK_LOW_8);
                print_char(ch);
            }
        } break;

        case TrapVector::PUTSP: {
            print_on_new_line();
            // Loop over words, then split into bytes
            // This is done to ensure the memory check is sound
            for (Word i = registers.general_purpose[0];; ++i) {
                const Word word = memory_checked(i);
                OK_OR_RETURN();

                const char high = static_cast<char>(bits_high(word));
                const char low = static_cast<char>(bits_low(word));
                if (high == 0x00)
                    break;
                print_char(high);
                if (low == 0x00)
                    break;
                print_char(low);
            }
        }; break;

        case TrapVector::HALT:
            do_halt = true;
            return;
            break;

        case TrapVector::REG:
            print_registers();
            break;

        default:
            fprintf(stderr, "Invalid trap vector 0x%02x\n",
                    static_cast<Word>(trap_vector));
            ERROR = ERR_MALFORMED_TRAP;
            return;
    }
}

void read_obj_filename_to_memory(const char *const obj_filename) {
    size_t words_read;

    FILE *obj_file;
    if (obj_filename[0] == '\0') {
        // Already checked for stdin-input in assemble+execute mode
        obj_file = stdin;
    } else {
        obj_file = fopen(obj_filename, "rb");
        if (obj_file == nullptr) {
            fprintf(stderr, "Could not open file %s\n", obj_filename);
            ERROR = ERR_FILE_OPEN;
            return;
        }
    }

    Word origin;
    words_read =
        fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, obj_file);

    if (ferror(obj_file)) {
        fprintf(stderr, "Could not read file %s\n", obj_filename);
        ERROR = ERR_FILE_READ;
        return;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_filename);
        ERROR = ERR_FILE_TOO_SHORT;
        return;
    }

    Word start = swap_endian(origin);

    /* printf("origin: 0x%04x\n", start); */

    char *const memory_at_file = reinterpret_cast<char *>(memory + start);
    const size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    words_read = fread(memory_at_file, WORD_SIZE, max_file_bytes, obj_file);

    if (ferror(obj_file)) {
        fprintf(stderr, "Could not read file %s\n", obj_filename);
        ERROR = ERR_FILE_READ;
        return;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_filename);
        ERROR = ERR_FILE_TOO_SHORT;
        return;
    }
    if (!feof(obj_file)) {
        fprintf(stderr, "File is too long %s\n", obj_filename);
        ERROR = ERR_FILE_TOO_LONG;
        return;
    }

    Word end = start + words_read;

    for (size_t i = 0; i < start; ++i)
        memory[i] = 0;
    for (size_t i = start; i < end; ++i)
        memory[i] = swap_endian(memory[i]);
    for (size_t i = end; i < MEMORY_SIZE; ++i)
        memory[i] = 0;

    /* printf("words read: %ld\n", words_read); */

    memory_file_bounds.start = start;
    memory_file_bounds.end = end;

    fclose(obj_file);
}

// Check memory address is within the 'allocated' file memory
Word &memory_checked(Word addr) {
    if (addr < memory_file_bounds.start) {
        fprintf(stderr, "Cannot access non-user memory (before user memory)\n");
        ERROR = ERR_ADDRESS_TOO_LOW;
    }
    if (addr > MEMORY_USER_MAX) {
        fprintf(stderr, "Cannot access non-user memory (after user memory)\n");
        ERROR = ERR_ADDRESS_TOO_HIGH;
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

void print_char(char ch) {
    if (ch == '\r') {
        ch = '\n';
    }
    printf("%c", ch);
    stdout_on_new_line = ch == '\n';
}

void print_on_new_line() {
    if (!stdout_on_new_line) {
        printf("\n");
        stdout_on_new_line = true;
    }
}

// Since %b printf format specifier is ""not ISO-compliant""
static char *halfbyte_string(const Word word) {
    static char str[5];
    for (int i = 0; i < 4; ++i) {
        str[i] = '0' + ((word >> (3 - i)) & 0b1);
    }
    str[4] = '\0';
    return str;
}

void print_registers() {
    const int width = 27;
    const char *const box_h = "─";
    const char *const box_v = "│";
    const char *const box_tl = "╭";
    const char *const box_tr = "╮";
    const char *const box_bl = "╰";
    const char *const box_br = "╯";

    print_on_new_line();

    printf("  %s", box_tl);
    for (size_t i = 0; i < width; ++i)
        printf("%s", box_h);
    printf("%s\n", box_tr);

    printf("  %s ", box_v);
    printf("pc: 0x%04hx        cc: %x%x%x", registers.program_counter,
           (registers.condition >> 2),        // Negative
           (registers.condition >> 1) & 0b1,  // Zero
           (registers.condition) & 0b1);      // Positive
    printf(" %s\n", box_v);

    printf("  %s ", box_v);
    printf("       HEX    UINT    INT");
    printf(" %s\n", box_v);

    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        const Word value = registers.general_purpose[reg];
        printf("  %s ", box_v);
        printf("r%d  0x%04hx  %6hd  %5hu", reg, value, value, value);
        printf(" %s\n", box_v);
    }

    printf("  %s", box_bl);
    for (size_t i = 0; i < width; ++i)
        printf("%s", box_h);
    printf("%s\n", box_br);

    stdout_on_new_line = true;
}

#endif
