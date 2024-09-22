#ifndef ASSEMBLE_CPP
#define ASSEMBLE_CPP

#include <cctype>   // isspace
#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // strcmp, strncmp
#include <vector>   // std::vector

#include "bitmasks.hpp"
#include "error.hpp"
#include "globals.hpp"
#include "slice.cpp"
#include "token.cpp"
#include "types.hpp"

using std::vector;

// This can be large as it is never aggregated
#define MAX_LINE 512  // Includes '\0'

// TODO(chore): Document functions
// TODO(chore): Move all function doc comments to prototypes ?
// TODO(refactor): Change some out-params to be return values

void assemble(
    const char *const asm_filename, const ObjectFile &output, Error &error
);
// Used by `assemble`
void write_obj_file(
    const char *const filename, const vector<Word> &words, Error &error
);
void assemble_file_to_words(
    const char *const filename, vector<Word> &words, Error &error
);

// Used by `assemble_file_to_words`
void parse_line(
    vector<Word> &words,
    const char *&line,
    vector<LabelDefinition> &label_definitions,
    vector<LabelReference> &label_references,
    int line_number,
    bool &is_end,
    bool &failed
);
void parse_directive(
    vector<Word> &words,
    const char *&line,
    const Directive directive,
    bool &is_end,
    bool &failed
);
void parse_instruction(
    Word &word,
    const char *&line,
    const Instruction &instruction,
    const size_t word_index,
    vector<LabelReference> &label_references,
    const int line_number,
    bool &failed
);

void print_invalid_operand(
    const char *const expected,
    const TokenKind token_kind,
    Instruction instruction
);
void expect_next_token(const char *&line, Token &token, bool &failed);
void expect_next_token_after_comma(
    const char *&line, Token &token, bool &failed
);
void expect_token_is_kind(
    const Token &token, const enum TokenKind kind, bool &failed
);
void expect_integer_fits_size(
    InitialSignWord integer, size_t size_bits, bool &failed
);
void expect_line_eol(const char *line, bool &failed);

uint8_t get_branch_condition_code(const Instruction instruction);
TrapVector get_trap_vector(const Instruction instruction);
void add_label_reference(
    vector<LabelReference> &references,
    const StringSlice &name,
    const Word index,
    const int line_number,
    const bool is_offset11
);
bool find_label_definition(
    const LabelString &target,
    const vector<LabelDefinition> &definitions,
    SignedWord &index
);
char escape_character(const char ch, bool &failed);

bool does_integer_fit_size(
    const InitialSignWord integer, const uint8_t size_bits
);
bool does_integer_fit_size_inner(
    const SignedWord integer, const uint8_t size_bits
);

void assemble(
    const char *const asm_filename, const ObjectFile &output, Error &error
) {
    vector<Word> words;
    assemble_file_to_words(asm_filename, words, error);
    OK_OR_RETURN(error);

    if (output.kind == ObjectFile::FILE) {
        write_obj_file(output.filename, words, error);
        OK_OR_RETURN(error);
    } else {
        // TODO(refactor): Write to memory in `assemble_file_to_words`
        //      Saves a redundant copy of the array
        const Word origin = words[0];
        for (size_t i = 1; i < words.size(); ++i) {
            memory[origin + i - 1] = words[i];
        }
        memory_file_bounds.start = origin;
        memory_file_bounds.end = origin + words.size() - 1;
    }
}

void write_obj_file(
    const char *const filename, const vector<Word> &words, Error &error
) {
    FILE *obj_file;
    if (filename[0] == '\0') {
        // Already checked erroneous stdout-output in assemble+execute mode
        obj_file = stdout;
    } else {
        obj_file = fopen(filename, "wb");
        if (obj_file == nullptr) {
            fprintf(
                stderr, "Failed to open output file for writing: %s\n", filename
            );
            SET_ERROR(error, FILE);
            return;
        }
    }

    for (size_t i = 0; i < words.size(); ++i) {
        const Word word = swap_endian(words[i]);
        fwrite(&word, sizeof(Word), 1, obj_file);
        if (ferror(obj_file)) {
            SET_ERROR(error, FILE);
            return;
        }
    }

    fclose(obj_file);
}

void assemble_file_to_words(
    const char *const filename, vector<Word> &words, Error &error
) {
    // File errors are fatal to assembly process, all other errors can be
    // 'ignored' to allow parsing to continue to following lines. However, if
    // any error occurs, the program will stop after parsing, and not write the
    // output file (or execute, in ax mode).

    FILE *asm_file;
    if (filename[0] == '\0') {
        asm_file = stdin;
    } else {
        asm_file = fopen(filename, "r");
        if (asm_file == nullptr) {
            fprintf(
                stderr,
                "Failed to open assembly file for reading: %s\n",
                filename
            );
            SET_ERROR(error, FILE);
            return;
        }
    }

    vector<LabelDefinition> label_definitions;
    vector<LabelReference> label_references;

    bool is_end = false;  // Set to `true` by `.END`

    char line_buf[MAX_LINE];  // Buffer gets overwritten
    for (int line_number = 1; !is_end; ++line_number) {
        const char *line = line_buf;  // Pointer address is mutated

        if (fgets(line_buf, MAX_LINE, asm_file) == NULL)
            break;
        if (ferror(asm_file)) {
            SET_ERROR(error, FILE);
            return;
        }

        bool failed = false;
        parse_line(
            words,
            line,
            label_definitions,
            label_references,
            line_number,
            is_end,
            failed
        );

        if (failed) {
            fprintf(stderr, "\tLine %d\n", line_number);
            SET_ERROR(error, ASSEMBLE);
        }
    }

    if (!is_end) {
        fprintf(stderr, "File does not contain `.END` directive\n");
        SET_ERROR(error, ASSEMBLE);
    }

    // Replace label references with PC offsets based on label definitions
    for (size_t i = 0; i < label_references.size(); ++i) {
        const LabelReference &ref = label_references[i];

        SignedWord index;
        if (!find_label_definition(ref.name, label_definitions, index)) {
            fprintf(stderr, "Undefined label '%s'\n", ref.name);
            fprintf(stderr, "\tLine %d\n", ref.line_number);
            SET_ERROR(error, ASSEMBLE);
            continue;
        }

        const uint8_t size = (ref.is_offset11 ? 11 : 9);
        const Word mask = (1U << size) - 1;

        const SignedWord pc_offset =
            index - static_cast<SignedWord>(ref.index) - 1;
        if (!does_integer_fit_size_inner(pc_offset, size)) {
            fprintf(
                stderr,
                "Label '%s' is too far away to be referenced\n",
                ref.name
            );
            fprintf(stderr, "\tLine %d\n", ref.line_number);
            SET_ERROR(error, ASSEMBLE);
            continue;
        }

        words[ref.index] |= pc_offset & mask;
    }

    fclose(asm_file);
}

void parse_line(
    vector<Word> &words,
    const char *&line,
    vector<LabelDefinition> &label_definitions,
    vector<LabelReference> &label_references,
    int line_number,
    bool &is_end,
    bool &failed
) {
    Token token;
    take_next_token(line, token, failed);
    RETURN_IF_FAILED(failed);

    // Empty line (including line with only whitespace or a comment)
    if (token.kind == TokenKind::EOL)
        return;

    if (words.size() == 0) {
        if (token.kind != TokenKind::DIRECTIVE) {
            fprintf(stderr, "First line must be `.ORIG` directive\n");
            failed = true;
            // Silence this error message for following lines
            // Compilation will not succeed regardless

            words.push_back(0x0000);
            return;
        }
        take_next_token(line, token, failed);
        RETURN_IF_FAILED(failed);
        // Must be unsigned
        if (token.kind != TokenKind::INTEGER || token.value.integer.is_signed) {
            fprintf(
                stderr, "Positive integer literal required after `.ORIG`\n"
            );
            failed = true;
            return;
        }
        expect_line_eol(line, failed);
        RETURN_IF_FAILED(failed);
        words.push_back(token.value.integer.value);
        return;
    }

    if (token.kind == TokenKind::LABEL) {
        const StringSlice &name = token.value.label;
        const size_t index = words.size();

        for (size_t i = 0; i < label_definitions.size(); ++i) {
            if (string_equals_slice(label_definitions[i].name, name)) {
                fprintf(stderr, "Multiple labels are defined with the name '");
                print_string_slice(stderr, name);
                fprintf(stderr, "'\n");
                failed = true;
                return;
            }
            if (label_definitions[i].index == index) {
                fprintf(stderr, "Label defined on already-labelled line '");
                print_string_slice(stderr, name);
                fprintf(stderr, "'\n");
                failed = true;
                // Don't return, so that label still gets defined
            }
        }

        label_definitions.push_back({});
        LabelDefinition &def = label_definitions.back();
        // Label length has already been checked
        copy_string_slice_to_string(def.name, name);
        def.index = index;

        // Continue to instruction/directive after label
        take_next_token(line, token, failed);
        RETURN_IF_FAILED(failed);
        // Skip if colon following label name
        if (token.kind == TokenKind::COLON) {
            take_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
        }
    }

    if (token.kind == TokenKind::DIRECTIVE) {
        parse_directive(words, line, token.value.directive, is_end, failed);
        RETURN_IF_FAILED(failed);
        expect_line_eol(line, failed);
        RETURN_IF_FAILED(failed);
        return;  // Next line
    }

    // Line with only label
    if (token.kind == TokenKind::EOL)
        return;

    if (token.kind != TokenKind::INSTRUCTION) {
        fprintf(
            stderr,
            "Unexpected %s. Expected instruction or end of line\n",
            token_kind_to_string(token.kind)
        );
        failed = true;
        return;
    }

    const Instruction instruction = token.value.instruction;
    Word word;
    parse_instruction(
        word,
        line,
        instruction,
        words.size(),
        label_references,
        line_number,
        failed
    );
    RETURN_IF_FAILED(failed);
    expect_line_eol(line, failed);
    RETURN_IF_FAILED(failed);
    words.push_back(word);
}

void parse_directive(
    vector<Word> &words,
    const char *&line,
    const Directive directive,
    bool &is_end,
    bool &failed
) {
    Token token;

    switch (directive) {
        case Directive::ORIG:
            fprintf(stderr, "Unexpected `.ORIG` directive\n");
            failed = true;
            return;

        case Directive::END:
            is_end = true;
            // Ignore all following tokens, including on the same line
            return;

        case Directive::FILL: {
            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::INTEGER, failed);
            RETURN_IF_FAILED(failed);
            // Don't check integer size -- it should have been checked
            //     to fit in a word when token was parsed
            // Sign is ignored
            words.push_back(token.value.integer.value);
        }; break;

        case Directive::BLKW: {
            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind != TokenKind::INTEGER ||
                token.value.integer.is_signed) {
                fprintf(
                    stderr,
                    "Positive integer literal required after `.BLKW` "
                    "directive\n"
                );
                failed = true;
                return;
            }
            // Don't check integer size
            // Don't reserve space -- it's not worth it
            for (Word i = 0; i < token.value.integer.value; ++i) {
                words.push_back(0x0000);
            }
        }; break;

        case Directive::STRINGZ: {
            take_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind != TokenKind::STRING) {
                fprintf(
                    stderr,
                    "String literal required after `.STRINGZ` directive\n"
                );
                failed = true;
                return;
            }
            const char *string = token.value.string.pointer;
            for (size_t i = 0; i < token.value.string.length; ++i) {
                char ch = string[i];
                if (ch == '\\') {
                    ++i;
                    // "... \" is treated as unterminated
                    if (i > token.value.string.length) {
                        fprintf(stderr, "Unterminated string literal\n");
                        failed = true;
                        return;
                    }
                    ch = escape_character(string[i], failed);
                    RETURN_IF_FAILED(failed);
                }
                words.push_back(static_cast<Word>(ch));
            }
            words.push_back(0x0000);  // Null-termination
        }; break;
    }
}

void parse_instruction(
    Word &word,
    const char *&line,
    const Instruction &instruction,
    const size_t word_index,
    vector<LabelReference> &label_references,
    const int line_number,
    bool &failed
) {
    Token token;
    Opcode opcode;
    Word operands = 0x0000;

    switch (instruction) {
        case Instruction::ADD:
        case Instruction::AND: {
            opcode =
                instruction == Instruction::ADD ? Opcode::ADD : Opcode::AND;

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register src_reg_a = token.value.register_;
            operands |= src_reg_a << 6;

            // TODO(feat): Replace `expect_token_is_kind` with
            //     `print_invalid_operand` or something

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind == TokenKind::REGISTER) {
                const Register src_reg_b = token.value.register_;
                operands |= src_reg_b;
            } else if (token.kind == TokenKind::INTEGER) {
                const InitialSignWord immediate = token.value.integer;
                expect_integer_fits_size(immediate, 5, failed);
                RETURN_IF_FAILED(failed);
                operands |= 1 << 5;  // Flag
                operands |= immediate.value & BITMASK_LOW_5;
            } else {
                print_invalid_operand(
                    "integer or label", token.kind, instruction
                );
                failed = true;
                return;
            }
        }; break;

        case Instruction::NOT: {
            opcode = Opcode::NOT;

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register src_reg = token.value.register_;
            operands |= src_reg << 6;

            operands |= BITMASK_LOW_6;  // Padding
        }; break;

        case Instruction::BR:
        case Instruction::BRN:
        case Instruction::BRZ:
        case Instruction::BRP:
        case Instruction::BRNZ:
        case Instruction::BRZP:
        case Instruction::BRNP:
        case Instruction::BRNZP: {
            opcode = Opcode::BR;
            const uint8_t condition = get_branch_condition_code(instruction);
            operands |= condition << 9;

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9, failed);
                RETURN_IF_FAILED(failed);
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(
                    label_references,
                    token.value.label,
                    word_index,
                    line_number,
                    false
                );
            } else {
                print_invalid_operand(
                    "integer or label", token.kind, instruction
                );
                failed = true;
                return;
            }
        }; break;

        case Instruction::JMP:
        case Instruction::RET: {
            opcode = Opcode::JMP_RET;

            Register addr_reg = 7;  // Default R7 for `RET`
            if (instruction == Instruction::JMP) {
                expect_next_token(line, token, failed);
                RETURN_IF_FAILED(failed);
                expect_token_is_kind(token, TokenKind::REGISTER, failed);
                RETURN_IF_FAILED(failed);
                addr_reg = token.value.register_;
            }
            operands |= addr_reg << 6;
        }; break;

        case Instruction::JSR:
        case Instruction::JSRR: {
            opcode = Opcode::JSR_JSRR;

            if (instruction == Instruction::JSR) {
                operands |= 1 << 11;  // Flag

                // PCOffset11
                expect_next_token(line, token, failed);
                RETURN_IF_FAILED(failed);
                if (token.kind == TokenKind::INTEGER) {
                    // 11 bits
                    expect_integer_fits_size(token.value.integer, 11, failed);
                    RETURN_IF_FAILED(failed);
                    operands |= token.value.integer.value & BITMASK_LOW_11;
                } else if (token.kind == TokenKind::LABEL) {
                    add_label_reference(
                        label_references,
                        token.value.label,
                        word_index,
                        line_number,
                        true
                    );
                } else {
                    fprintf(stderr, "Invalid operand\n");
                    failed = true;
                    return;
                }
            } else {
                expect_next_token(line, token, failed);
                RETURN_IF_FAILED(failed);
                expect_token_is_kind(token, TokenKind::REGISTER, failed);
                RETURN_IF_FAILED(failed);
                const Register addr_reg = token.value.register_;
                operands |= addr_reg << 6;
            }
        }; break;

        case Instruction::LD:
        case Instruction::LDI:
        case Instruction::ST:
        case Instruction::STI: {
            switch (instruction) {
                case Instruction::LD:
                    opcode = Opcode::LD;
                    break;
                case Instruction::LDI:
                    opcode = Opcode::LDI;
                    break;
                case Instruction::ST:
                    opcode = Opcode::ST;
                    break;
                case Instruction::STI:
                    opcode = Opcode::STI;
                    break;
                default:
                    UNREACHABLE();
            }

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register ds_reg = token.value.register_;
            operands |= ds_reg << 9;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9, failed);
                RETURN_IF_FAILED(failed);
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(
                    label_references,
                    token.value.label,
                    word_index,
                    line_number,
                    false
                );
            } else {
                fprintf(stderr, "Invalid operand\n");
                failed = true;
                return;
            }
        }; break;

        case Instruction::LDR:
        case Instruction::STR: {
            opcode =
                instruction == Instruction::LDR ? Opcode::LDR : Opcode::STR;

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register ds_reg = token.value.register_;
            operands |= ds_reg << 9;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register base_reg = token.value.register_;
            operands |= base_reg << 6;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::INTEGER, failed);
            RETURN_IF_FAILED(failed);

            const InitialSignWord immediate = token.value.integer;
            // 6 bits
            expect_integer_fits_size(immediate, 6, failed);
            RETURN_IF_FAILED(failed);
            operands |= immediate.value & BITMASK_LOW_6;
        }; break;

        case Instruction::LEA: {
            opcode = Opcode::LEA;

            expect_next_token(line, token, failed);
            RETURN_IF_FAILED(failed);
            expect_token_is_kind(token, TokenKind::REGISTER, failed);
            RETURN_IF_FAILED(failed);
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line, token, failed);
            RETURN_IF_FAILED(failed);
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9, failed);
                RETURN_IF_FAILED(failed);
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(
                    label_references,
                    token.value.label,
                    word_index,
                    line_number,
                    false
                );
            } else {
                fprintf(stderr, "Invalid operand\n");
                failed = true;
                return;
            }
        }; break;

        case Instruction::TRAP:
        case Instruction::GETC:
        case Instruction::OUT:
        case Instruction::PUTS:
        case Instruction::IN:
        case Instruction::PUTSP:
        case Instruction::HALT:
        case Instruction::REG: {
            opcode = Opcode::TRAP;

            uint8_t trap_vector;
            switch (instruction) {
                // Trap instruction with explicit code
                case Instruction::TRAP: {
                    expect_next_token(line, token, failed);
                    RETURN_IF_FAILED(failed);
                    // Don't allow explicit sign
                    if (token.kind != TokenKind::INTEGER ||
                        token.value.integer.is_signed) {
                        fprintf(
                            stderr,
                            "Positive integer literal required after "
                            "`TRAP` instruction\n"
                        );
                        failed = true;
                        return;
                    }
                    const InitialSignWord immediate = token.value.integer;
                    // 8 bits -- always positive
                    // This incurs a redundant sign check, this is fine
                    expect_integer_fits_size(immediate, 8, failed);
                    RETURN_IF_FAILED(failed);
                    // TODO(correctness): What happens if cast fails?
                    trap_vector = immediate.value;
                }; break;

                default:
                    trap_vector =
                        static_cast<uint8_t>(get_trap_vector(instruction));
            }
            operands = static_cast<Word>(trap_vector);
        }; break;

        case Instruction::RTI: {
            opcode = Opcode::RTI;
        }; break;
    }

    word = static_cast<Word>(opcode) << 12 | operands;
}

void print_invalid_operand(
    const char *const expected,
    const TokenKind token_kind,
    Instruction instruction
) {
    fprintf(
        stderr,
        "Unexpected %s. Expected %s operand for `%s` instruction\n",
        token_kind_to_string(token_kind),
        expected,
        instruction_to_string(instruction)
    );
}

void expect_next_token(const char *&line, Token &token, bool &failed) {
    take_next_token(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind == TokenKind::EOL) {
        fprintf(stderr, "Expected operand\n");
        failed = true;
    }
}

void expect_next_token_after_comma(
    const char *&line, Token &token, bool &failed
) {
    take_next_token(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind == TokenKind::COMMA) {
        take_next_token(line, token, failed);
        RETURN_IF_FAILED(failed);
    }
    if (token.kind == TokenKind::EOL) {
        fprintf(stderr, "Expected operand\n");
        failed = true;
    }
}

// TODO(refactor): Remove these wrapper functions when error handling is good

void expect_token_is_kind(
    const Token &token, const enum TokenKind kind, bool &failed
) {
    if (token.kind != kind) {
        fprintf(stderr, "Invalid operand\n");
        failed = true;
    }
}

void expect_integer_fits_size(
    InitialSignWord integer, size_t size_bits, bool &failed
) {
    if (!does_integer_fit_size(integer, size_bits)) {
        fprintf(stderr, "Immediate too large\n");
        failed = true;
    }
}

void expect_line_eol(const char *line, bool &failed) {
    Token token;
    take_next_token(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind != TokenKind::EOL) {
        fprintf(stderr, "Unexpected operand after instruction\n");
        failed = true;
    }
}

// Must ONLY be called with a BR* instruction
uint8_t get_branch_condition_code(const Instruction instruction) {
    switch (instruction) {
        case Instruction::BRN:
            return 0b100;
        case Instruction::BRZ:
            return 0b010;
        case Instruction::BRP:
            return 0b001;
        case Instruction::BRNZ:
            return 0b110;
        case Instruction::BRZP:
            return 0b011;
        case Instruction::BRNP:
            return 0b101;
        case Instruction::BR:
        case Instruction::BRNZP:
            return 0b111;
        default:
            UNREACHABLE();
    }
}

// Must ONLY be called with trap instruction (`GETC`, `PUTS`, etc)
TrapVector get_trap_vector(const Instruction instruction) {
    switch (instruction) {
        case Instruction::GETC:
            return TrapVector::GETC;
        case Instruction::OUT:
            return TrapVector::OUT;
        case Instruction::PUTS:
            return TrapVector::PUTS;
        case Instruction::IN:
            return TrapVector::IN;
        case Instruction::PUTSP:
            return TrapVector::PUTSP;
        case Instruction::HALT:
            return TrapVector::HALT;
        case Instruction::REG:
            return TrapVector::REG;
        default:
            UNREACHABLE();
    }
}

void add_label_reference(
    vector<LabelReference> &references,
    const StringSlice &name,
    const Word index,
    const int line_number,
    const bool is_offset11
) {
    references.push_back({});
    LabelReference &ref = references.back();
    // Label length has already been checked
    copy_string_slice_to_string(ref.name, name);
    ref.index = index;
    ref.line_number = line_number;
    ref.is_offset11 = is_offset11;
}

bool find_label_definition(
    const LabelString &target,
    const vector<LabelDefinition> &definitions,
    SignedWord &index
) {
    for (size_t j = 0; j < definitions.size(); ++j) {
        const LabelDefinition candidate = definitions[j];
        // Use `strcasecmp` as both arguments are null-terminated and unknown
        //     length, and comparison is case-insensitive
        if (!strcasecmp(candidate.name, target)) {
            index = candidate.index;
            return true;
        }
    }
    return false;
}

char escape_character(const char ch, bool &failed) {
    switch (ch) {
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case '0':
            return '\0';
        default:
            fprintf(stderr, "Invalid escape sequence '\\%c'\n", ch);
            failed = true;
            return 0x7f;
    }
}

bool does_negative_integer_fit_size(
    const SignedWord integer, const uint8_t size_bits
) {
    // Flip sign and check against largest allowed negative value
    // Eg. size = 5
    //     Largest positive value: 0000'1111
    //     Largest negative value: 0001'0000 = max
    const Word max = 1 << (size_bits - 1);
    return (Word)(-integer) <= max;
}
bool does_positive_integer_fit_size(
    const Word integer, const uint8_t size_bits
) {
    // Check if any bits above --and including-- sign bit are set
    return integer >> (size_bits - 1) == 0;
}

bool does_integer_fit_size(
    const InitialSignWord integer, const uint8_t size_bits
) {
    if (integer.is_signed) {
        return does_negative_integer_fit_size(integer.value, size_bits);
    }
    return does_positive_integer_fit_size(integer.value, size_bits);
}

// TODO(refactor): Rename these functions PLEASE !!!
bool does_integer_fit_size_inner(
    const SignedWord integer, const uint8_t size_bits
) {
    if (integer < 0) {
        return does_negative_integer_fit_size(integer, size_bits);
    }
    return does_positive_integer_fit_size(integer, size_bits);
}

#endif
