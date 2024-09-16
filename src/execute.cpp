#ifndef EXECUTE_CPP
#define EXECUTE_CPP

#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memset

#include "bitmasks.hpp"
#include "error.hpp"
#include "globals.hpp"
#include "tty.cpp"
#include "types.hpp"

#define _to_sext_word(_value, _size) \
    (sign_extend(static_cast<SignedWord>(_value), (_size)))
#define low_5_bits_sext(_instr) (_to_sext_word((_instr) & BITMASK_LOW_5, 5))
#define low_6_bits_sext(_instr) (_to_sext_word((_instr) & BITMASK_LOW_6, 6))
#define low_9_bits_sext(_instr) (_to_sext_word((_instr) & BITMASK_LOW_9, 9))
#define low_11_bits_sext(_instr) (_to_sext_word((_instr) & BITMASK_LOW_11, 11))

// Prompt for `IN` trap
#define TRAP_IN_PROMPT "Input a character: "

// TODO(refactor): Re-order functions

void execute(const char *const obj_filename, Error &error);
void execute_next_instrution(bool &do_halt, Error &error);
void execute_trap_instruction(const Word instr, bool &do_halt, Error &error);

void read_obj_filename_to_memory(const char *const obj_filename, Error &error);

Word &memory_checked(Word addr, Error &error);

SignedWord sign_extend(SignedWord value, const size_t size);
void set_condition_codes(const Word result);
void print_char(char ch);
void print_on_new_line(void);
static char *halfbyte_string(const Word word);
void print_registers(void);
char condition_char(ConditionCode condition);

void execute(const char *const obj_filename, Error &error) {
    read_obj_filename_to_memory(obj_filename, error);
    OK_OR_RETURN(error);

    // GP and condition registers are already initialized to 0
    registers.program_counter = memory_file_bounds.start;

    // Loop until `true` is returned, indicating a HALT (TRAP 0x25)
    bool do_halt = false;
    while (!do_halt) {
        execute_next_instrution(do_halt, error);
        OK_OR_RETURN(error);
    }

    if (!stdout_on_new_line)
        printf("\n");
}

// `true` return value indicates that program should end
void execute_next_instrution(bool &do_halt, Error &error) {
    memory_checked(registers.program_counter, error);
    OK_OR_RETURN(error);

    const Word instr = memory[registers.program_counter];
    ++registers.program_counter;

    // May be invalid enum variant
    // Handled in default switch branch
    const Opcode opcode = static_cast<Opcode>(bits_12_15(instr));

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
                    SET_ERROR(error, EXECUTE);
                    return;
                }
                const Register src_reg_b = bits_0_2(instr);
                value_b = static_cast<SignedWord>(
                    registers.general_purpose[src_reg_b]);
            } else {
                value_b = low_5_bits_sext(instr);
            }

            const Word result = static_cast<Word>(value_a + value_b);
            registers.general_purpose[dest_reg] = result;
            set_condition_codes(result);
        }; break;

        // AND*
        case Opcode::AND: {
            const Register dest_reg = bits_9_11(instr);
            const Register src_reg_a = bits_6_8(instr);

            // TODO: These maybe should be `Word` ? Shouldn't matter I think...
            const SignedWord value_a = registers.general_purpose[src_reg_a];
            SignedWord value_b;

            if (bit_5(instr) == 0b0) {
                // 2 bits padding
                const uint8_t padding = bits_3_4(instr);
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for AND instruction\n");
                    SET_ERROR(error, EXECUTE);
                    return;
                }
                const Register src_reg_b = bits_0_2(instr);
                value_b = registers.general_purpose[src_reg_b];
            } else {
                value_b = low_5_bits_sext(instr);
            }

            const Word result = static_cast<Word>(value_a & value_b);
            registers.general_purpose[dest_reg] = result;
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
                SET_ERROR(error, EXECUTE);
                return;
            }

            const Word result = ~(registers.general_purpose[src_reg]);
            registers.general_purpose[dest_reg] = result;
            set_condition_codes(result);
        }; break;

        // BRcc
        case Opcode::BR: {
            // Skip special NOP case
            if (instr == 0x0000)
                break;

            const ConditionCode condition = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);

            // If any bits of the condition codes match
            // Instruction `BR` is equivalent to `BRnzp`, but 0b000 is checked
            //     here for completeness (as if 0b111)
            if (condition == 0b000 ||
                (condition & registers.condition) != 0b000) {
                registers.program_counter += offset;
            }
        }; break;

        // JMP/RET
        case Opcode::JMP_RET: {
            // 3 bits padding
            const uint8_t padding_1 = bits_9_11(instr);
            if (padding_1 != 0b000) {
                fprintf(stderr,
                        "Expected padding 0b000 for JMP/RET instruction\n");
                SET_ERROR(error, EXECUTE);
                return;
            }
            // 6 bits padding
            // After base register
            const uint8_t padding_2 = bits_0_6(instr);
            if (padding_2 != 0b000000) {
                fprintf(stderr,
                        "Expected padding 0b000000 for JMP/RET instruction\n");
                SET_ERROR(error, EXECUTE);
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
                const SignedWord offset = low_11_bits_sext(instr);
                registers.program_counter += offset;
            } else {
                // JSRR
                // 2 bits padding
                const uint8_t padding = bits_9_10(instr);
                if (padding != 0b00) {
                    fprintf(stderr,
                            "Expected padding 0b00 for JSRR instruction\n");
                    SET_ERROR(error, EXECUTE);
                    return;
                }

                const Register base_reg = bits_6_8(instr);
                const Word base = registers.general_purpose[base_reg];
                registers.program_counter = base;
            }
        }; break;

        // LD*
        case Opcode::LD: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);

            const Word value =
                memory_checked(registers.program_counter + offset, error);
            OK_OR_RETURN(error);
            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // ST
        case Opcode::ST: {
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);

            const Word value = registers.general_purpose[src_reg];
            memory_checked(registers.program_counter + offset, error) = value;
            OK_OR_RETURN(error);
        }; break;

        // LDR*
        case Opcode::LDR: {
            const Register dest_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_sext(instr);

            const Word base = registers.general_purpose[base_reg];
            const Word value = memory_checked(base + offset, error);
            OK_OR_RETURN(error);

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STR
        case Opcode::STR: {
            const Register src_reg = bits_9_11(instr);
            const Register base_reg = bits_6_8(instr);
            const SignedWord offset = low_6_bits_sext(instr);

            const Word base = registers.general_purpose[base_reg];
            const Word value = registers.general_purpose[src_reg];

            memory_checked(base + offset, error) = value;
            OK_OR_RETURN(error);
        }; break;

        // LDI*
        case Opcode::LDI: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);

            const Word pointer =
                memory_checked(registers.program_counter + offset, error);
            OK_OR_RETURN(error);
            const Word value = memory_checked(pointer, error);
            OK_OR_RETURN(error);

            registers.general_purpose[dest_reg] = value;
            set_condition_codes(value);
        }; break;

        // STI
        case Opcode::STI: {
            const Register src_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);

            const Word pointer =
                memory_checked(registers.program_counter + offset, error);
            OK_OR_RETURN(error);
            const Word value = registers.general_purpose[src_reg];

            memory_checked(pointer, error) = value;
            OK_OR_RETURN(error);
        }; break;

        // LEA*
        case Opcode::LEA: {
            const Register dest_reg = bits_9_11(instr);
            const SignedWord offset = low_9_bits_sext(instr);
            const Word addr =
                static_cast<Word>(registers.program_counter + offset);
            registers.general_purpose[dest_reg] = addr;
            set_condition_codes(addr);
        }; break;

        // TRAP
        case Opcode::TRAP: {
            execute_trap_instruction(instr, do_halt, error);
            OK_OR_RETURN(error);
        }; break;

        // RTI (supervisor-only)
        case Opcode::RTI:
            fprintf(stderr,
                    "Invalid use of RTI opcode: 0b%s in non-supervisor mode\n",
                    halfbyte_string(static_cast<Word>(opcode)));
            SET_ERROR(error, EXECUTE);
            return;
            break;

        // Invalid enum variant
        default:
            fprintf(stderr, "Invalid opcode: 0b%s (0x%04x)\n",
                    halfbyte_string(static_cast<Word>(opcode)),
                    static_cast<Word>(opcode));
            SET_ERROR(error, EXECUTE);
            return;
    }
}

void execute_trap_instruction(const Word instr, bool &do_halt, Error &error) {
    // 4 bits padding
    const uint8_t padding = bits_8_12(instr);
    if (padding != 0b0000) {
        fprintf(stderr, "Expected padding 0x00 for TRAP instruction\n");
        SET_ERROR(error, EXECUTE);
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
            print_on_new_line();
            registers.general_purpose[0] = input;
        }; break;

        case TrapVector::IN: {
            print_on_new_line();
            printf(TRAP_IN_PROMPT);
            tty_nobuffer_noecho();
            const char input = getchar() & BITMASK_LOW_8;  // Zero high 8 bits
            tty_restore();
            print_char(input);
            print_on_new_line();
            registers.general_purpose[0] = input;
        }; break;

        case TrapVector::OUT: {
            const Word word = registers.general_purpose[0];
            // TODO(correctness): Should it be low 8-bits instead ?
            const char ch = static_cast<char>(word & BITMASK_LOW_7);
            print_char(ch);
        }; break;

        case TrapVector::PUTS: {
            print_on_new_line();
            for (Word i = registers.general_purpose[0];; ++i) {
                const Word word = memory_checked(i, error);
                OK_OR_RETURN(error);

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
                const Word word = memory_checked(i, error);
                OK_OR_RETURN(error);

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
            SET_ERROR(error, EXECUTE);
            return;
    }
}

void read_obj_filename_to_memory(const char *const obj_filename, Error &error) {
    size_t words_read;

    FILE *obj_file;
    if (obj_filename[0] == '\0') {
        // Already checked erroneous stdin-input in assemble+execute mode
        obj_file = stdin;
    } else {
        obj_file = fopen(obj_filename, "rb");
        if (obj_file == nullptr) {
            fprintf(stderr, "Could not open file %s\n", obj_filename);
            SET_ERROR(error, EXECUTE);
            return;
        }
    }

    Word origin;
    words_read =
        fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, obj_file);

    if (ferror(obj_file)) {
        fprintf(stderr, "Could not read file %s\n", obj_filename);
        SET_ERROR(error, EXECUTE);
        return;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_filename);
        SET_ERROR(error, EXECUTE);
        return;
    }

    Word start = swap_endian(origin);

    char *const memory_at_file = reinterpret_cast<char *>(memory + start);
    const size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    words_read = fread(memory_at_file, WORD_SIZE, max_file_bytes, obj_file);

    if (ferror(obj_file)) {
        fprintf(stderr, "Could not read file %s\n", obj_filename);
        SET_ERROR(error, EXECUTE);
        return;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", obj_filename);
        SET_ERROR(error, EXECUTE);
        return;
    }
    if (!feof(obj_file)) {
        fprintf(stderr, "File is too long %s\n", obj_filename);
        SET_ERROR(error, EXECUTE);
        return;
    }

    Word end = start + words_read;

    for (size_t i = 0; i < start; ++i)
        memory[i] = 0;
    for (size_t i = start; i < end; ++i)
        memory[i] = swap_endian(memory[i]);
    for (size_t i = end; i < MEMORY_SIZE; ++i)
        memory[i] = 0;

    memory_file_bounds.start = start;
    memory_file_bounds.end = end;

    fclose(obj_file);
}

// Check memory address is within the 'allocated' file memory
Word &memory_checked(Word addr, Error &error) {
    if (addr < memory_file_bounds.start) {
        fprintf(stderr, "Cannot access non-user memory (before user memory)\n");
        SET_ERROR(error, EXECUTE);
    }
    if (addr > MEMORY_USER_MAX) {
        fprintf(stderr, "Cannot access non-user memory (after user memory)\n");
        SET_ERROR(error, EXECUTE);
    }
    return memory[addr];
}

// TODO(fix): Truncate to `size` bits in this function, don't rely on caller
SignedWord sign_extend(SignedWord value, const size_t size) {
    // If previous-highest bit is set
    // Set all bits higher than previous sign bit to 1
    // TODO(refactor): This could be done without a branch lol
    if (value >> (size - 1) & 0b1)
        return value | (~0U << size);
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
    if (ch == '\r')
        ch = '\n';
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
    printf("pc: 0x%04hx          cc: %c", registers.program_counter,
           condition_char(registers.condition));
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

char condition_char(ConditionCode condition) {
    switch (condition) {
        case 0b100:
            return 'N';
        case 0b010:
            return 'Z';
        case 0b001:
            return 'P';
        default:
            return '?';
    }
}

#endif
