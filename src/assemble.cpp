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

using std::vector;

// This can be large as it is never aggregated
#define MAX_LINE 512  // Includes '\0'
#define MAX_LABEL 32  // Includes '\0'

// Must be a copied string, as `line` is overwritten
typedef char LabelString[MAX_LABEL];

typedef struct LabelDefinition {
    LabelString name;
    Word index;
} LabelDefinition;

typedef struct LabelReference {
    LabelString name;
    Word index;
    bool is_offset11;  // Used for `JSR` only
} LabelReference;

enum class Directive {
    ORIG,
    END,
    FILL,
    BLKW,
    STRINGZ,
};

// MUST match order of `Directive` enum
static const char *const DIRECTIVE_NAMES[] = {
    "ORIG", "END", "FILL", "BLKW", "STRINGZ",
};

enum class Instruction {
    ADD,
    AND,
    NOT,
    BR,
    BRN,
    BRZ,
    BRP,
    BRNZ,
    BRZP,
    BRNP,
    BRNZP,
    JMP,
    RET,
    JSR,
    JSRR,
    LD,
    ST,
    LDI,
    STI,
    LDR,
    STR,
    LEA,
    TRAP,
    GETC,
    OUT,
    PUTS,
    IN,
    PUTSP,
    HALT,
    REG,  // Extension trap
    RTI,  // Only used in 'supervisor' mode
};

// MUST match order of `Instruction` enum
static const char *const INSTRUCTION_NAMES[] = {
    "ADD",  "AND",  "NOT",   "BR",    "BRN",  "BRZ", "BRP",  "BRNZ",
    "BRZP", "BRNP", "BRNZP", "JMP",   "RET",  "JSR", "JSRR", "LD",
    "ST",   "LDI",  "STI",   "LDR",   "STR",  "LEA", "TRAP", "GETC",
    "OUT",  "PUTS", "IN",    "PUTSP", "HALT", "REG", "RTI",
};

// Can be signed or unsigned
// Intended sign needs to be known to check if integer is too large for
//     a particular instruction
// Value is stored unsigned if `!is_signed` (high bit does not imply negative)
// Value is stored signed if `is_signed`, in 2's compliment, but stored in an
//     unsigned type
// TODO(refactor): Rename this type
typedef struct SignExplicitWord {
    Word value;
    bool is_signed;
} SignExplicitWord;

enum class TokenKind {
    DIRECTIVE,
    INSTRUCTION,
    REGISTER,
    INTEGER,
    STRING,
    LABEL,
    COMMA,
    COLON,
    NONE,
};
typedef struct Token {
    TokenKind kind;
    union {
        Directive directive;
        Instruction instruction;
        Register register_;
        // Sign depends on if `-` character is present in asm file
        SignExplicitWord integer;
        // `StringSlice`s are only valid for lifetime of `line`
        StringSlice string;
        StringSlice label;  // Gets copied on push to a labels vector
    } value;
} Token;

// TODO(chore): Document functions
// TODO(chore): Move all function doc comments to prototypes ?

void assemble(const char *const asm_filename, const char *const obj_filename);
// Used by `assemble`
void write_obj_file(const char *const filename, const vector<Word> &words);
void assemble_file_to_words(const char *const filename, vector<Word> &words);

// Used by `assemble_file_to_words`
void parse_directive(vector<Word> &words, const char *&line_ptr,
                     const Directive directive, bool &is_end);
void parse_instruction(Word &word, const char *&line_ptr,
                       const Instruction &instruction, const size_t word_count,
                       vector<LabelReference> &label_references);
void expect_next_token(const char *&line_ptr, Token &token);
void expect_next_token_after_comma(const char *&line_ptr, Token &token);
void expect_token_is_kind(const Token &token, const enum TokenKind kind);
void expect_integer_fits_size(SignExplicitWord integer, size_t size_bits);
void expect_line_eol(const char *line_ptr);

ConditionCode get_branch_condition_code(const Instruction instruction);
void add_label_reference(vector<LabelReference> &references,
                         const StringSlice &name, const Word index,
                         const bool is_offset11);
bool find_label_definition(const LabelString &target,
                           const vector<LabelDefinition> &definitions,
                           SignedWord &index);
char escape_character(const char ch);
bool does_integer_fit_size(const SignExplicitWord integer,
                           const uint8_t size_bits);

// Note: 'take' here means increment the line pointer and return a token
void take_next_token(const char *&line, Token &token);
// Used by `take_next_token`
void take_literal_string(const char *&line, Token &token);
void take_directive(const char *&line, Token &token);
void take_register(const char *&line, Token &token);
void take_integer_hex(const char *&line, Token &token);
void take_integer_decimal(const char *&line, Token &token);
int8_t parse_hex_digit(const char ch);
bool append_decimal_digit_checked(Word &number, uint8_t digit,
                                  bool is_negative);
bool is_char_eol(const char ch);
bool is_char_valid_in_identifier(const char ch);
bool is_char_valid_identifier_start(const char ch);

// Directive/Instruction to/from string
static const char *directive_to_string(const Directive directive);
void directive_from_string(Token &token, const StringSlice directive);
static const char *instruction_to_string(const Instruction instruction);
bool try_instruction_from_string_slice(Token &token,
                                       const StringSlice &instruction);

// Debugging
void _print_token(const Token &token);

void assemble(const char *const asm_filename, const char *const obj_filename) {
    vector<Word> words;
    assemble_file_to_words(asm_filename, words);
    OK_OR_RETURN();
    write_obj_file(obj_filename, words);
    OK_OR_RETURN();
}

void write_obj_file(const char *const filename, const vector<Word> &words) {
    FILE *obj_file;
    if (filename[0] == '\0') {
        // Already checked for stdout-output in assemble+execute mode
        obj_file = stdout;
    } else {
        obj_file = fopen(filename, "wb");
        if (obj_file == nullptr) {
            fprintf(stderr, "Could not open file %s\n", filename);
            ERROR = ERR_FILE_OPEN;
            return;
        }
    }

    /* printf("Size: %lu words\n", words.size()); */

    for (size_t i = 0; i < words.size(); ++i) {
        const Word word = swap_endian(words[i]);
        fwrite(&word, sizeof(Word), 1, obj_file);
        if (ferror(obj_file)) {
            ERROR = ERR_FILE_WRITE;
            return;
        }
    }

    fclose(obj_file);
}

void assemble_file_to_words(const char *const filename, vector<Word> &words) {
    FILE *asm_file;
    if (filename[0] == '\0') {
        asm_file = stdin;
    } else {
        asm_file = fopen(filename, "r");
        if (asm_file == nullptr) {
            fprintf(stderr, "Could not open file %s\n", filename);
            ERROR = ERR_FILE_OPEN;
            return;
        }
    }

    vector<LabelDefinition> label_definitions;
    vector<LabelReference> label_references;

    bool is_end = false;  // Set to `true` by `.END`

    char line_buf[MAX_LINE];  // Buffer gets overwritten
    while (!is_end) {
        const char *line_ptr = line_buf;  // Pointer address is mutated

        if (fgets(line_buf, MAX_LINE, asm_file) == NULL)
            break;
        if (ferror(asm_file)) {
            ERROR = ERR_FILE_READ;
            return;
        }

        /* printf("<%s>\n", line_ptr); */

        Token token;
        take_next_token(line_ptr, token);
        OK_OR_RETURN();

        /* printf("----\n"); */
        /* _print_token(token); */

        // Empty line
        if (token.kind == TokenKind::NONE)
            continue;

        if (words.size() == 0) {
            /* _print_token(token); */
            if (token.kind != TokenKind::DIRECTIVE) {
                fprintf(stderr, "First line must be `.ORIG`\n");
                ERROR = ERR_ASM_EXPECTED_ORIG;
                return;
            }
            take_next_token(line_ptr, token);
            OK_OR_RETURN();
            // Must be unsigned
            if (token.kind != TokenKind::INTEGER ||
                token.value.integer.is_signed) {
                fprintf(stderr,
                        "Positive integer literal required after `.ORIG`\n");
                ERROR = ERR_ASM_EXPECTED_OPERAND;
                return;
            }
            words.push_back(token.value.integer.value);
            continue;
        }

        // TODO(correctness): Parsing label before directive *might* have bad
        //     effects, although I cannot think of any off the top of my head.
        if (token.kind == TokenKind::LABEL) {
            const StringSlice &name = token.value.label;
            for (size_t i = 0; i < label_definitions.size(); ++i) {
                /* printf("-- <"); */
                /* printf("%s", label_definitions[i].name); */
                /* printf("> : <"); */
                /* print_string_slice(stdout, name); */
                /* printf(">\n"); */
                if (!strncmp(label_definitions[i].name, name.pointer,
                             name.length)) {
                    fprintf(stderr, "Duplicate label '");
                    print_string_slice(stderr, name);
                    fprintf(stderr, "'\n");
                    ERROR = ERR_ASM_DUPLICATE_LABEL;
                    return;
                }
            }

            label_definitions.push_back({});
            LabelDefinition &def = label_definitions.back();
            // Label length has already been checked
            copy_string_slice_to_string(def.name, name);
            def.index = words.size();

            // Continue to instruction/directive after label
            take_next_token(line_ptr, token);
            OK_OR_RETURN();
            // Skip if colon following label name
            if (token.kind == TokenKind::COLON) {
                take_next_token(line_ptr, token);
                OK_OR_RETURN();
            }
        }

        /* _print_token(token); */

        if (token.kind == TokenKind::DIRECTIVE) {
            parse_directive(words, line_ptr, token.value.directive, is_end);
            expect_line_eol(line_ptr);
            OK_OR_RETURN();
            continue;
        }

        // Line with only label
        if (token.kind == TokenKind::NONE)
            continue;

        if (token.kind != TokenKind::INSTRUCTION) {
            /* _print_token(token); */
            fprintf(stderr, "Expected instruction or end of line\n");
            ERROR = ERR_ASM_EXPECTED_INSTRUCTION;
            return;
        }

        const Instruction instruction = token.value.instruction;
        /* _print_token(token); */
        Word word;
        parse_instruction(word, line_ptr, instruction, words.size(),
                          label_references);
        /* printf("%s\n", line_ptr); */
        expect_line_eol(line_ptr);
        OK_OR_RETURN();
        words.push_back(word);
    }

    if (!is_end) {
        fprintf(stderr, "Missing `.END` directive\n");
        ERROR = ERR_ASM_EXPECTED_END;
        return;
    }

    // Replace label references with PC offsets based on label definitions
    for (size_t i = 0; i < label_references.size(); ++i) {
        const LabelReference &ref = label_references[i];
        /* printf("Resolving '%s' at 0x%04x\n", ref.name, ref.index); */

        SignedWord index;
        if (!find_label_definition(ref.name, label_definitions, index)) {
            fprintf(stderr, "Undefined label '%s'\n", ref.name);
            ERROR = ERR_ASM_UNDEFINED_LABEL;
            return;
        }
        /* printf("Found definition at 0x%04lx\n", index); */

        /* printf("0x%04x -> 0x%04x\n", ref.index, index); */
        const SignedWord pc_offset = index - ref.index - 1;
        /* printf("PC offset: 0x%04lx\n", pc_offset); */

        const Word mask = (1U << (ref.is_offset11 ? 11 : 9)) - 1;

        words[ref.index] |= pc_offset & mask;
    }

    fclose(asm_file);
}

void parse_directive(vector<Word> &words, const char *&line_ptr,
                     const Directive directive, bool &is_end) {
    Token token;

    switch (directive) {
        case Directive::ORIG:
            fprintf(stderr, "Unexpected `.ORIG`\n");
            ERROR = ERR_ASM_UNEXPECTED_DIRECTIVE;
            return;

        case Directive::END:
            is_end = true;
            return;

        case Directive::FILL: {
            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::INTEGER);
            OK_OR_RETURN();
            // Don't check integer size -- it should have been checked
            //     to fit in a word when being
            // Sign is ignored
            words.push_back(token.value.integer.value);
        }; break;

        case Directive::BLKW: {
            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            if (token.kind != TokenKind::INTEGER ||
                token.value.integer.is_signed) {
                fprintf(stderr,
                        "Positive integer literal required after `.BLKW`\n");
                ERROR = ERR_ASM_EXPECTED_OPERAND;
                return;
            }
            // Don't check integer size
            // Don't reserve space -- it's not worth it
            for (Word i = 0; i < token.value.integer.value; ++i) {
                words.push_back(0x0000);
            }
        }; break;

        case Directive::STRINGZ: {
            /* printf("%s\n", line_ptr); */
            take_next_token(line_ptr, token);
            OK_OR_RETURN();
            /* _print_token(token); */
            if (token.kind != TokenKind::STRING) {
                fprintf(stderr, "String literal required after `.STRINGZ`\n");
                ERROR = ERR_ASM_EXPECTED_OPERAND;
                return;
            }
            const char *string = token.value.string.pointer;
            for (size_t i = 0; i < token.value.string.length; ++i) {
                char ch = string[i];
                if (ch == '\\') {
                    ++i;
                    // "... \" is treated as unterminated
                    if (i > token.value.string.length) {
                        ERROR = ERR_ASM_UNTERMINATED_STRING;
                        return;
                    }
                    ch = escape_character(string[i]);
                    OK_OR_RETURN();
                }
                words.push_back(static_cast<Word>(ch));
            }
            words.push_back(0x0000);  // Null-termination
        }; break;
    }
}

void parse_instruction(Word &word, const char *&line_ptr,
                       const Instruction &instruction, const size_t word_count,
                       vector<LabelReference> &label_references) {
    Token token;
    Opcode opcode;
    Word operands = 0x0000;

    switch (instruction) {
        case Instruction::ADD:
        case Instruction::AND: {
            opcode =
                instruction == Instruction::ADD ? Opcode::ADD : Opcode::AND;

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register src_reg_a = token.value.register_;
            operands |= src_reg_a << 6;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            if (token.kind == TokenKind::REGISTER) {
                const Register src_reg_b = token.value.register_;
                operands |= src_reg_b;
            } else {
                expect_token_is_kind(token, TokenKind::INTEGER);
                OK_OR_RETURN();
                const SignExplicitWord immediate = token.value.integer;
                expect_integer_fits_size(immediate, 5);
                OK_OR_RETURN();
                operands |= 1 << 5;  // Flag
                operands |= immediate.value & BITMASK_LOW_5;
            }
        }; break;

        case Instruction::NOT: {
            opcode = Opcode::NOT;

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
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
            const ConditionCode condition =
                get_branch_condition_code(instruction);
            operands |= condition << 9;

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9);
                OK_OR_RETURN();
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(label_references, token.value.label,
                                    word_count, false);
            } else {
                fprintf(stderr, "Invalid operand\n");
                ERROR = ERR_ASM_INVALID_OPERAND;
                return;
            }
        }; break;

        case Instruction::JMP:
        case Instruction::RET: {
            opcode = Opcode::JMP_RET;

            Register addr_reg = 7;  // Default R7 for `RET`
            if (instruction == Instruction::JMP) {
                expect_next_token(line_ptr, token);
                OK_OR_RETURN();
                expect_token_is_kind(token, TokenKind::REGISTER);
                OK_OR_RETURN();
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
                expect_next_token(line_ptr, token);
                OK_OR_RETURN();
                if (token.kind == TokenKind::INTEGER) {
                    // 11 bits
                    expect_integer_fits_size(token.value.integer, 11);
                    OK_OR_RETURN();
                    operands |= token.value.integer.value & BITMASK_LOW_11;
                } else if (token.kind == TokenKind::LABEL) {
                    add_label_reference(label_references, token.value.label,
                                        word_count, false);
                } else {
                    fprintf(stderr, "Invalid operand\n");
                    ERROR = ERR_ASM_INVALID_OPERAND;
                    return;
                }
            } else {
                expect_next_token(line_ptr, token);
                OK_OR_RETURN();
                expect_token_is_kind(token, TokenKind::REGISTER);
                OK_OR_RETURN();
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

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register ds_reg = token.value.register_;
            operands |= ds_reg << 9;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9);
                OK_OR_RETURN();
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(label_references, token.value.label,
                                    word_count, false);
            } else {
                fprintf(stderr, "Invalid operand\n");
                ERROR = ERR_ASM_INVALID_OPERAND;
                return;
            }
        }; break;

        case Instruction::LDR:
        case Instruction::STR: {
            opcode =
                instruction == Instruction::LDR ? Opcode::LDR : Opcode::STR;

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register ds_reg = token.value.register_;
            operands |= ds_reg << 9;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register base_reg = token.value.register_;
            operands |= base_reg << 6;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            expect_token_is_kind(token, TokenKind::INTEGER);
            OK_OR_RETURN();

            const SignExplicitWord immediate = token.value.integer;
            // 6 bits
            expect_integer_fits_size(immediate, 6);
            OK_OR_RETURN();
            operands |= immediate.value & BITMASK_LOW_6;
        }; break;

        case Instruction::LEA: {
            opcode = Opcode::LEA;

            expect_next_token(line_ptr, token);
            OK_OR_RETURN();
            /* _print_token(token); */
            expect_token_is_kind(token, TokenKind::REGISTER);
            OK_OR_RETURN();
            const Register dest_reg = token.value.register_;
            operands |= dest_reg << 9;

            expect_next_token_after_comma(line_ptr, token);
            OK_OR_RETURN();
            /* _print_token(token); */
            if (token.kind == TokenKind::INTEGER) {
                // 9 bits
                expect_integer_fits_size(token.value.integer, 9);
                OK_OR_RETURN();
                operands |= token.value.integer.value & BITMASK_LOW_9;
            } else if (token.kind == TokenKind::LABEL) {
                add_label_reference(label_references, token.value.label,
                                    word_count, false);
            } else {
                fprintf(stderr, "Invalid operand\n");
                ERROR = ERR_ASM_INVALID_OPERAND;
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

            TrapVector trap_vector;
            switch (instruction) {
                case Instruction::GETC:
                    trap_vector = TrapVector::GETC;
                    break;
                case Instruction::OUT:
                    trap_vector = TrapVector::OUT;
                    break;
                case Instruction::PUTS:
                    trap_vector = TrapVector::PUTS;
                    break;
                case Instruction::IN:
                    trap_vector = TrapVector::IN;
                    break;
                case Instruction::PUTSP:
                    trap_vector = TrapVector::PUTSP;
                    break;
                case Instruction::HALT:
                    trap_vector = TrapVector::HALT;
                    break;
                case Instruction::REG:
                    trap_vector = TrapVector::REG;
                    break;

                // Trap instruction with explicit code
                case Instruction::TRAP: {
                    expect_next_token(line_ptr, token);
                    OK_OR_RETURN();
                    // Don't allow explicit sign
                    if (token.kind != TokenKind::INTEGER ||
                        token.value.integer.is_signed) {
                        fprintf(
                            stderr,
                            "Positive integer literal required after `TRAP`\n");
                        ERROR = ERR_ASM_EXPECTED_OPERAND;
                        return;
                    }
                    const SignExplicitWord immediate = token.value.integer;
                    // 8 bits -- always positive
                    // TODO(refactor): A redundant check happens here. Maybe
                    //     this could be avoided ?
                    expect_integer_fits_size(immediate, 8);
                    OK_OR_RETURN();
                    trap_vector = static_cast<TrapVector>(immediate.value);
                }; break;

                default:
                    UNREACHABLE();
            }
            operands = static_cast<Word>(trap_vector);
        }; break;

        case Instruction::RTI: {
            opcode = Opcode::RTI;
        }; break;
    }

    word = static_cast<Word>(opcode) << 12 | operands;
}

void expect_next_token(const char *&line_ptr, Token &token) {
    take_next_token(line_ptr, token);
    OK_OR_RETURN();
    if (token.kind == TokenKind::NONE) {
        fprintf(stderr, "Expected operand\n");
        ERROR = ERR_ASM_EXPECTED_OPERAND;
    }
}

void expect_next_token_after_comma(const char *&line_ptr, Token &token) {
    take_next_token(line_ptr, token);
    OK_OR_RETURN();
    if (token.kind == TokenKind::COMMA) {
        take_next_token(line_ptr, token);
        OK_OR_RETURN();
    }
    if (token.kind == TokenKind::NONE) {
        fprintf(stderr, "Expected operand\n");
        ERROR = ERR_ASM_EXPECTED_OPERAND;
    }
}

// TODO(refactor): Remove these wrapper functions when error handling is good

void expect_token_is_kind(const Token &token, const enum TokenKind kind) {
    if (token.kind != kind) {
        fprintf(stderr, "Invalid operand\n");
        ERROR = ERR_ASM_INVALID_OPERAND;
    }
}

void expect_integer_fits_size(SignExplicitWord integer, size_t size_bits) {
    if (!does_integer_fit_size(integer, size_bits)) {
        fprintf(stderr, "Immediate too large\n");
        ERROR = ERR_ASM_IMMEDIATE_TOO_LARGE;
    }
}

void expect_line_eol(const char *line_ptr) {
    Token token;
    take_next_token(line_ptr, token);
    OK_OR_RETURN();
    if (token.kind != TokenKind::NONE) {
        fprintf(stderr, "Unexpected operand after instruction\n");
        ERROR = ERR_ASM_UNEXPECTED_OPERAND;
    }
}

// Must ONLY be called with a BR* instruction
ConditionCode get_branch_condition_code(const Instruction instruction) {
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

void add_label_reference(vector<LabelReference> &references,
                         const StringSlice &name, const Word index,
                         const bool is_offset11) {
    references.push_back({});
    LabelReference &ref = references.back();
    /* printf("\nStart:\n"); */
    /* for (size_t i = 0; i < name.length; ++i) { */
    /* printf("%ld\t", i); */
    /* fflush(stdout); */
    /* printf("%c\n", name.pointer[i]); */
    /* ref.name[i] = 'i'; */
    /* } */
    /* printf("Done!\n"); */

    // Label length has already been checked
    copy_string_slice_to_string(ref.name, name);
    ref.index = index;
    ref.is_offset11 = is_offset11;

    /* printf("label:<"); */
    /* _print_string_slice(name); */
    /* printf(">\n"); */
}

bool find_label_definition(const LabelString &target,
                           const vector<LabelDefinition> &definitions,
                           SignedWord &index) {
    for (size_t j = 0; j < definitions.size(); ++j) {
        const LabelDefinition candidate = definitions[j];
        // Use `strcmp` as both arguments are null-terminated and unknown length
        if (!strcmp(candidate.name, target)) {
            index = candidate.index;
            return true;
        }
    }
    return false;
}

char escape_character(const char ch) {
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
            ERROR = ERR_ASM_INVALID_ESCAPE_CHAR;
            return 0x7f;
    }
}

bool does_integer_fit_size(const SignExplicitWord integer,
                           const uint8_t size_bits) {
    // TODO(rewrite): There has to be a better way to do this...
    if (integer.is_signed) {
        // Flip sign and check against largest allowed negative value
        // Eg. size = 5
        //     Largest positive value: 0000'1111
        //     Largest negative value: 0001'0000 = max
        const Word max = 1 << (size_bits - 1);
        return (Word)(-integer.value) <= max;
    }
    // Check if any bits above --and including-- sign bit are set
    return integer.value >> (size_bits - 1) == 0;
}

void take_next_token(const char *&line, Token &token) {
    token.kind = TokenKind::NONE;

    /* printf("-- <%s>\n", line); */

    // Ignore leading spaces
    while (isspace(line[0]))
        ++line;
    // Linebreak, EOF, or comment
    if (is_char_eol(line[0]))
        return;

    /* printf("<<%s>>\n", line); */

    /* printf("<%c> 0x%04hx\n", line[0], line[0]); */

    // Comma can appear between operands
    if (line[0] == ',') {
        token.kind = TokenKind::COMMA;
        ++line;
        return;
    }
    // Colon can appear following label declaration
    if (line[0] == ':') {
        token.kind = TokenKind::COLON;
        ++line;
        return;
    }

    // String literal
    take_literal_string(line, token);
    OK_OR_RETURN();
    if (token.kind != TokenKind::NONE)
        return;  // Tried to parse, but failed

    // Register
    take_register(line, token);  // Cannot fail
    OK_OR_RETURN();

    // Directive
    take_directive(line, token);
    OK_OR_RETURN();
    if (token.kind != TokenKind::NONE)
        return;  // Tried to parse, but failed

    // Hex literal
    take_integer_hex(line, token);
    OK_OR_RETURN();
    if (token.kind != TokenKind::NONE)
        return;  // Tried to parse, but failed
    // Decimal literal
    take_integer_decimal(line, token);
    OK_OR_RETURN();
    if (token.kind != TokenKind::NONE)
        return;  // Tried to parse, but failed

    // Character cannot start an identifier -> invalid
    if (!is_char_valid_identifier_start(line[0])) {
        ERROR = ERR_ASM_INVALID_TOKEN;
        return;
    }

    // Label or instruction
    StringSlice identifier;
    identifier.pointer = line;
    ++line;
    while (is_char_valid_in_identifier(tolower(line[0])))
        ++line;
    identifier.length = line - identifier.pointer;

    /* printf("IDENT: <"); */
    /* _print_string_slice(identifier); */
    /* printf(">\n"); */

    // Sets kind and value, if is valid instruction
    if (!try_instruction_from_string_slice(token, identifier)) {
        // Label
        if (identifier.length >= MAX_LABEL) {
            fprintf(stderr, "Label too long.\n");
            ERROR = ERR_ASM_LABEL_TOO_LONG;
            return;
        }
        token.kind = TokenKind::LABEL;
        token.value.label = identifier;
    }
}

void take_literal_string(const char *&line, Token &token) {
    if (line[0] != '"')
        return;
    ++line;  // Opening quote

    token.kind = TokenKind::STRING;
    token.value.string.pointer = line;
    for (; line[0] != '"'; ++line) {
        // String cannot be multi-line, or unclosed within a file
        if (line[0] == '\n' || line[0] == '\0') {
            ERROR = ERR_ASM_UNTERMINATED_STRING;
            return;
        }
    }

    token.value.string.length = line - token.value.string.pointer;
    ++line;  // Closing quote
}

void take_directive(const char *&line, Token &token) {
    if (line[0] != '.')
        return;
    ++line;  // '.'

    StringSlice directive;
    directive.pointer = line;

    // Use `isalpha` because directives only ever use letters, unlike labels
    while (isalpha(line[0]))
        ++line;
    directive.length = line - directive.pointer;

    // Sets kind and value
    directive_from_string(token, directive);
    // ^ This can fail, but `ERROR` should be checked by caller
}

void take_register(const char *&line, Token &token) {
    if (line[0] != 'R' && line[0] != 'r')
        return;

    if (!isdigit(line[1]) || line[1] - '0' >= GP_REGISTER_COUNT)
        return;

    // Token is actually the start of a label, such as `R2Foo`
    if (is_char_valid_in_identifier(line[2]))
        return;

    ++line;  // [rR]
    token.kind = TokenKind::REGISTER;
    token.value.register_ = line[0] - '0';
    ++line;  // [0-9]
    return;
}

void take_integer_hex(const char *&line, Token &token) {
    const char *new_line = line;

    bool is_signed = false;
    if (new_line[0] == '-') {
        ++new_line;
        is_signed = true;
    }
    // Only allow one 0 in prefixx
    if (new_line[0] == '0')
        ++new_line;
    // Must have prefix
    if (new_line[0] != 'x' && new_line[0] != 'X') {
        return;
    }
    ++new_line;

    if (new_line[0] == '-') {
        ++new_line;
        // Don't allow `-x-`
        if (is_signed) {
            ERROR = ERR_ASM_INVALID_TOKEN;
            return;
        }
        is_signed = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1]))
        ++new_line;

    // Not an integer
    // Continue to next token
    if (parse_hex_digit(new_line[0]) < 0) {
        return;
    }

    line = new_line;  // Skip [x0-] which was just checked
    token.kind = TokenKind::INTEGER;
    token.value.integer.is_signed = is_signed;

    Word number = 0;
    for (size_t i = 0;; ++i) {
        const char ch = line[0];
        const int8_t digit = parse_hex_digit(ch);  // Catches '\0'
        if (digit < 0) {
            // Checks if number is immediately followed by identifier character
            //     (like a suffix), which is invalid
            if (ch != '\0' && is_char_valid_in_identifier(ch)) {
                ERROR = ERR_ASM_INVALID_TOKEN;
                return;
            }
            break;
        }
        // Hex literals cannot be more than 4 digits
        // Leading zeros have already been skipped
        // Ignore sign
        if (i >= 4) {
            fprintf(stderr, "Immediate too large\n");
            ERROR = ERR_ASM_IMMEDIATE_TOO_LARGE;
            return;
        }
        number <<= 4;
        number += digit;
        ++line;
    }

    if (is_signed)
        number *= -1;  // Store negative number in unsigned word
    token.value.integer.value = number;
}

void take_integer_decimal(const char *&line, Token &token) {
    const char *new_line = line;

    bool is_signed = false;
    if (new_line[0] == '-') {
        ++new_line;
        is_signed = true;
    }
    // Don't allow any 0's before prefix
    if (new_line[0] == '#')
        ++new_line;  // Optional
    if (new_line[0] == '-') {
        ++new_line;
        // Don't allow `-#-`
        if (is_signed) {
            ERROR = ERR_ASM_INVALID_TOKEN;
            return;
        }
        is_signed = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1]))
        ++new_line;

    // Not an integer
    // Continue to next token
    if (!isdigit(new_line[0])) {
        return;
    }

    line = new_line;  // Skip [#0-] which was just checked
    token.kind = TokenKind::INTEGER;
    token.value.integer.is_signed = is_signed;

    Word number = 0;
    while (true) {
        const char ch = line[0];
        if (!isdigit(line[0])) {  // Catches '\0'
            // Checks if number is immediately followed by identifier character
            //     (like a suffix), which is invalid
            if (ch != '\0' && is_char_valid_in_identifier(ch)) {
                ERROR = ERR_ASM_INVALID_TOKEN;
                return;
            }
            break;
        }
        if (!append_decimal_digit_checked(number, ch - '0', is_signed)) {
            fprintf(stderr, "Immediate too large\n");
            ERROR = ERR_ASM_IMMEDIATE_TOO_LARGE;
            return;
        }
        ++line;
    }

    if (is_signed)
        number *= -1;  // Store negative number in unsigned word
    token.value.integer.value = number;
}

// Returns -1 if not a valid hex digit
int8_t parse_hex_digit(const char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 10;
    return -1;
}

bool append_decimal_digit_checked(Word &number, uint8_t digit,
                                  bool is_negative) {
    if (number > WORD_MAX_UNSIGNED / 10)
        return false;
    number *= 10;
    // Largest negative integer is 1 larger than largest positive integer
    if (number > WORD_MAX_UNSIGNED - digit + (is_negative ? 1 : 0))
        return false;
    number += digit;
    return true;
}

bool is_char_eol(const char ch) {
    // EOF, EOL, or comment
    return ch == '\0' || ch == '\r' || ch == '\n' || ch == ';';
}
bool is_char_valid_in_identifier(const char ch) {
    return ch == '_' || isalpha(ch) || isdigit(ch);
}
bool is_char_valid_identifier_start(const char ch) {
    return ch == '_' || isalpha(ch);
}

static const char *directive_to_string(const Directive directive) {
    return DIRECTIVE_NAMES[static_cast<size_t>(directive)];
}

void directive_from_string(Token &token, const StringSlice directive) {
    token.kind = TokenKind::DIRECTIVE;
    for (size_t i = 0; i < sizeof(DIRECTIVE_NAMES) / sizeof(char *); ++i) {
        if (string_equals_slice(DIRECTIVE_NAMES[i], directive)) {
            token.value.directive = static_cast<Directive>(i);
            return;
        }
    }
    ERROR = ERR_ASM_INVALID_DIRECTIVE;
}

static const char *instruction_to_string(const Instruction instruction) {
    return INSTRUCTION_NAMES[static_cast<size_t>(instruction)];
}

// TODO(refactor): Maybe return void and check .kind==TokenKind::Instruction
//     in caller
bool try_instruction_from_string_slice(Token &token,
                                       const StringSlice &instruction) {
    const size_t count =
        sizeof(INSTRUCTION_NAMES) / sizeof(INSTRUCTION_NAMES[0]);
    for (size_t i = 0; i < count; ++i) {
        if (string_equals_slice(INSTRUCTION_NAMES[i], instruction)) {
            token.kind = TokenKind::INSTRUCTION;
            token.value.instruction = static_cast<Instruction>(i);
            return true;
        }
    }
    return false;
}

void _print_token(const Token &token) {
    printf("Token: ");
    switch (token.kind) {
        case TokenKind::INSTRUCTION:
            printf("Instruction: %s\n",
                   instruction_to_string(token.value.instruction));
            break;
        case TokenKind::DIRECTIVE:
            printf("Directive: %s\n",
                   directive_to_string(token.value.directive));
            break;
        case TokenKind::REGISTER:
            printf("Register: R%d\n", token.value.register_);
            break;
        case TokenKind::INTEGER:
            if (token.value.integer.is_signed) {
                printf("Integer: 0x%04hx #%hd\n", token.value.integer.value,
                       token.value.integer.value);
            } else {
                printf("Integer: 0x%04hx #+%hu\n", token.value.integer.value,
                       token.value.integer.value);
            }
            break;
        case TokenKind::STRING:
            printf("String: <");
            print_string_slice(stdout, token.value.string);
            printf(">\n");
            break;
        case TokenKind::LABEL:
            printf("Label: <");
            print_string_slice(stdout, token.value.label);
            printf(">\n");
            break;
        case TokenKind::COMMA:
            printf("Comma\n");
            break;
        case TokenKind::COLON:
            printf("Colon\n");
            break;
        case TokenKind::NONE:
            printf("NONE!\n");
            break;
    }
}

#endif
