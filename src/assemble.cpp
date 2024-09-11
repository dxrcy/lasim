#ifndef ASSEMBLE_CPP
#define ASSEMBLE_CPP

#include <cctype>   // isspace
#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // strcmp, strncmp
#include <vector>   // std::vector

#include "bitmasks.hpp"
#include "error.hpp"
#include "types.hpp"

using std::vector;

#define MAX_LINE 512  // This can be large as it is not aggregated
#define MAX_LABEL 32

#define EXPECT_NEXT_TOKEN(line_ptr, token)               \
    {                                                    \
        RETURN_IF_ERR(take_next_token(line_ptr, token)); \
        if (token.kind == Token::NONE) {                 \
            fprintf(stderr, "Expected operand\n");       \
            return ERR_ASM_EXPECTED_OPERAND;             \
        }                                                \
    }

#define EXPECT_TOKEN_IS_KIND(token, kind)         \
    {                                             \
        if (token.kind != Token::kind) {          \
            fprintf(stderr, "Invalid operand\n"); \
            return ERR_ASM_INVALID_OPERAND;       \
        }                                         \
    }

#define EXPECT_COMMA(line_ptr)                                     \
    {                                                              \
        RETURN_IF_ERR(take_next_token(line_ptr, token));           \
        if (token.kind != Token::COMMA) {                          \
            fprintf(stderr, "Expected comma following operand\n"); \
            return ERR_ASM_EXPECTED_COMMA;                         \
        }                                                          \
    }

#define EXPECT_EOL(line_ptr)                                           \
    {                                                                  \
        Token token;                                                   \
        RETURN_IF_ERR(take_next_token(line_ptr, token));               \
        if (token.kind != Token::NONE) {                               \
            fprintf(stderr, "Unexpected operand after instruction\n"); \
            return ERR_ASM_UNEXPECTED_OPERAND;                         \
        }                                                              \
    }

// Temporary reference to a substring of a line
// Must be copied if used after line is overwritten
typedef struct StringSlice {
    const char *pointer;
    size_t length;
} StringSlice;

typedef char LabelString[MAX_LABEL];

// TODO: Why is `index` signed ?
typedef struct LabelDefinition {
    LabelString name;
    SignedWord index;
} LabelDefinition;

typedef struct LabelReference {
    LabelString name;
    SignedWord index;
    bool is_offset11;  // Used for `JSR` only
} LabelReference;

enum class Directive {
    ORIG,
    END,
    STRINGZ,
    FILL,
    BLKW,
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
    RTI,
};

typedef struct Token {
    enum {
        DIRECTIVE,
        INSTRUCTION,
        REGISTER,
        INTEGER,
        STRING,
        LABEL,
        COMMA,
        NONE,
    } kind;
    union {
        Directive directive;
        Instruction instruction;
        Register register_;
        SignedWord integer;
        // `StringSlice`s are only valid for lifetime of `line`
        StringSlice string;
        StringSlice label;  // Gets copied on push to a labels vector
    } value;
} Token;

// TODO: Document functions

// Note: 'take' here means increment the line pointer and return a token

Error assemble(const char *const asm_filename, const char *const obj_filename);
Error write_obj_file(const char *const filename, const vector<Word> &words);
Error assemble_file_to_words(const char *const filename, vector<Word> &words);

ConditionCode get_branch_condition_code(const Instruction instruction);
void add_label_reference(vector<LabelReference> &references,
                         const StringSlice &name, const Word index,
                         const bool is_offset11);
bool find_label_definition(const LabelString &target,
                           const vector<LabelDefinition> &definitions,
                           SignedWord &index);

Error take_next_token(const char *&line, Token &token);
Error take_integer_hex(const char *&line, Token &token);
Error take_integer_decimal(const char *&line, Token &token);
int8_t parse_hex_digit(const char ch);

Error escape_character(char *const ch);
bool is_char_eol(const char ch);
bool is_char_valid_in_identifier(const char ch);
bool is_char_valid_identifier_start(const char ch);

bool string_equals_slice(const char *const target, const StringSlice candidate);
void copy_string_slice_to_string(char *dest, const StringSlice src);
void print_string_slice(FILE *const &file, const StringSlice &slice);

static const char *directive_to_string(const Directive directive);
Error directive_from_string(Token &token, const StringSlice directive);
static const char *instruction_to_string(const Instruction instruction);
bool try_instruction_from_string_slice(Token &token,
                                       const StringSlice &instruction);

void _print_token(const Token &token);

Error assemble(const char *const asm_filename, const char *const obj_filename) {
    vector<Word> out_words;
    RETURN_IF_ERR(assemble_file_to_words(asm_filename, out_words));
    RETURN_IF_ERR(write_obj_file(obj_filename, out_words));
    return ERR_OK;
}

Error write_obj_file(const char *const filename, const vector<Word> &words) {
    FILE *const obj_file = fopen(filename, "wb");
    if (obj_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    /* printf("Size: %lu words\n", words.size()); */

    for (size_t i = 0; i < words.size(); ++i) {
        const Word word = swap_endian(words[i]);
        fwrite(&word, sizeof(Word), 1, obj_file);
        if (ferror(obj_file)) return ERR_FILE_WRITE;
    }

    fclose(obj_file);
    return ERR_OK;
}

Error assemble_file_to_words(const char *const filename, vector<Word> &words) {
    FILE *const asm_file = fopen(filename, "r");
    if (asm_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    vector<LabelDefinition> label_definitions;
    vector<LabelReference> label_references;

    char line_buf[MAX_LINE];  // Buffer gets overwritten
    while (true) {
        const char *line_ptr = line_buf;  // Pointer address is mutated

        if (fgets(line_buf, MAX_LINE, asm_file) == NULL) break;
        if (ferror(asm_file)) return ERR_FILE_READ;

        /* printf("<%s>\n", line_ptr); */

        Token token;
        RETURN_IF_ERR(take_next_token(line_ptr, token));

        /* _print_token(token); */

        // Empty line
        if (token.kind == Token::NONE) continue;

        if (words.size() == 0) {
            /* _print_token(token); */
            if (token.kind != Token::DIRECTIVE) {
                fprintf(stderr, "First line must be `.ORIG`\n");
                return ERR_ASM_EXPECTED_ORIG;
            }
            RETURN_IF_ERR(take_next_token(line_ptr, token));
            if (token.kind != Token::INTEGER) {
                fprintf(stderr, "Integer literal required after `.ORIG`\n");
                return ERR_ASM_EXPECTED_OPERAND;
            }
            words.push_back(token.value.integer);
            continue;
        }

        // TODO: Parsing label before directive *might* have bad effects,
        //     although I cannot think of any off the top of my head.
        if (token.kind == Token::LABEL) {
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
                    return ERR_ASM_DUPLICATE_LABEL;
                }
            }

            label_definitions.push_back({});
            LabelDefinition &def = label_definitions.back();
            copy_string_slice_to_string(def.name, name);
            def.index = words.size();

            // Continue to instruction/directive after label
            RETURN_IF_ERR(take_next_token(line_ptr, token));
        }

        /* _print_token(token); */

        if (token.kind == Token::DIRECTIVE) {
            switch (token.value.directive) {
                case Directive::END:
                    goto stop_parsing;

                case Directive::STRINGZ: {
                    /* printf("%s\n", line_ptr); */
                    RETURN_IF_ERR(take_next_token(line_ptr, token));
                    /* _print_token(token); */
                    if (token.kind != Token::STRING) {
                        fprintf(stderr,
                                "String literal required after `.STRINGZ`\n");
                        return ERR_ASM_EXPECTED_OPERAND;
                    }
                    const char *string = token.value.string.pointer;
                    for (size_t i = 0; i < token.value.string.length; ++i) {
                        char ch = string[i];
                        if (ch == '\\') {
                            ++i;
                            // "... \" is treated as unterminated
                            if (i > token.value.string.length)
                                return ERR_ASM_UNTERMINATED_STRING;
                            ch = string[i];
                            RETURN_IF_ERR(escape_character(&ch));
                        }
                        words.push_back(static_cast<Word>(ch));
                    }
                    words.push_back(0x0000);  // Null-termination
                }; break;

                case Directive::FILL: {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_KIND(token, INTEGER);
                    words.push_back(token.value.integer);
                }; break;

                case Directive::BLKW: {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_KIND(token, INTEGER);
                    // TODO: Replace with better vector function
                    for (Word i = 0; i < token.value.integer; ++i) {
                        words.push_back(0x0000);
                    }
                }; break;

                default:
                    // Includes `ORIG`
                    fprintf(stderr, "Unexpected directive `%s`\n",
                            directive_to_string(token.value.directive));
                    return ERR_ASM_UNEXPECTED_DIRECTIVE;
            }

            EXPECT_EOL(line_ptr);
            continue;
        }

        // Line with only label
        if (token.kind == Token::NONE) continue;

        if (token.kind != Token::INSTRUCTION) {
            /* _print_token(token); */
            fprintf(stderr, "Expected instruction or end of line\n");
            return ERR_ASM_EXPECTED_INSTRUCTION;
        }

        const Instruction instruction = token.value.instruction;
        /* printf("Instruction: %s\n", instruction_to_string(instruction)); */

        Opcode opcode;
        Word operands = 0x0000;

        switch (instruction) {
            case Instruction::ADD:
            case Instruction::AND: {
                opcode =
                    instruction == Instruction::ADD ? Opcode::ADD : Opcode::AND;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register src_reg_a = token.value.register_;
                operands |= src_reg_a << 6;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                if (token.kind == Token::REGISTER) {
                    const Register src_reg_b = token.value.register_;
                    operands |= src_reg_b;
                } else if (token.kind == Token::INTEGER) {
                    const SignedWord immediate = token.value.integer;
                    /* _print_token(token); */
                    // TODO: This currently treats all integers as unsigned!
                    // Ideally, if the integer is meant to be signed (it
                    //      has a minus sign), the last 5 bits may be 1s,
                    //      otherwise only the last 4 bits may be 1s
                    if (immediate >> 5 != 0) {
                        fprintf(stderr, "Immediate too large\n");
                        /* return ERR_ASM_IMMEDIATE_TOO_LARGE; */
                    }
                    operands |= 1 << 5;  // Flag
                    operands |= immediate & BITMASK_LOW_5;
                }
            }; break;

            case Instruction::NOT: {
                opcode = Opcode::NOT;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
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

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, LABEL);
                add_label_reference(label_references, token.value.label,
                                    words.size(), false);
            }; break;

            case Instruction::JMP:
            case Instruction::RET: {
                opcode = Opcode::JMP_RET;

                Register addr_reg = 7;  // Default R7 for `RET`
                if (instruction == Instruction::JMP) {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_KIND(token, REGISTER);
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
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_KIND(token, LABEL);
                    /* printf(">>"); */
                    /* _print_token(token); */
                    add_label_reference(label_references, token.value.label,
                                        words.size(), true);
                } else {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_KIND(token, REGISTER);
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

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register ds_reg = token.value.register_;
                operands |= ds_reg << 9;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, LABEL);
                add_label_reference(label_references, token.value.label,
                                    words.size(), false);
            }; break;

            case Instruction::LDR:
            case Instruction::STR: {
                opcode =
                    instruction == Instruction::LDR ? Opcode::LDR : Opcode::STR;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register ds_reg = token.value.register_;
                operands |= ds_reg << 9;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register base_reg = token.value.register_;
                operands |= base_reg << 6;
                EXPECT_COMMA(line_ptr);

                // TODO: Check should be aware of sign; See `ADD` case
                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, INTEGER);
                const SignedWord immediate = token.value.integer;
                if (immediate >> 6 != 0) {
                    fprintf(stderr, "Immediate too large\n");
                    return ERR_ASM_IMMEDIATE_TOO_LARGE;
                }
                operands |= immediate & BITMASK_LOW_6;
            }; break;

            case Instruction::LEA: {
                opcode = Opcode::LEA;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_KIND(token, LABEL);
                add_label_reference(label_references, token.value.label,
                                    words.size(), false);
            }; break;

            case Instruction::TRAP:
            case Instruction::GETC:
            case Instruction::OUT:
            case Instruction::PUTS:
            case Instruction::IN:
            case Instruction::PUTSP:
            case Instruction::HALT: {
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

                    // Trap instruction with explicit code
                    case Instruction::TRAP: {
                        // TODO: Check should be aware of sign; See `ADD`
                        // case
                        EXPECT_NEXT_TOKEN(line_ptr, token);
                        EXPECT_TOKEN_IS_KIND(token, INTEGER);
                        const SignedWord immediate = token.value.integer;
                        if (immediate >> 8 != 0) {
                            fprintf(stderr, "Immediate too large\n");
                            return ERR_ASM_IMMEDIATE_TOO_LARGE;
                        }
                        trap_vector = static_cast<TrapVector>(immediate);
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

        EXPECT_EOL(line_ptr);

        const Word word = static_cast<Word>(opcode) << 12 | operands;
        words.push_back(word);
    }
stop_parsing:

    // Replace label references with PC offsets based on label definitions
    for (size_t i = 0; i < label_references.size(); ++i) {
        const LabelReference &ref = label_references[i];
        /* printf("Resolving '%s' at 0x%04x\n", ref.name, ref.index); */

        SignedWord index;
        if (!find_label_definition(ref.name, label_definitions, index)) {
            fprintf(stderr, "Undefined label '%s'\n", ref.name);
            return ERR_ASM_UNDEFINED_LABEL;
        }
        /* printf("Found definition at 0x%04lx\n", index); */

        /* printf("0x%04x -> 0x%04x\n", ref.index, index); */
        const SignedWord pc_offset = index - ref.index - 1;
        /* printf("PC offset: 0x%04lx\n", pc_offset); */

        const Word mask = (1U << (ref.is_offset11 ? 11 : 9)) - 1;

        words[ref.index] |= pc_offset & mask;
    }

    fclose(asm_file);
    return ERR_OK;
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
    copy_string_slice_to_string(ref.name, name);
    ref.index = index;
    ref.is_offset11 = is_offset11;
    /* printf("label:<"); */
    /* _print_string_slice(name); */
    /* printf(">\n"); */
}

// TODO: Maybe return index, and return -1 if not found ?
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

Error take_next_token(const char *&line, Token &token) {
    token.kind = Token::NONE;

    /* printf("-- <%s>\n", line); */

    // Ignore leading spaces
    while (isspace(line[0])) ++line;
    // Linebreak, EOF, or comment
    if (is_char_eol(line[0])) return ERR_OK;

    /* printf("<<%s>>\n", line); */

    /* printf("<%c> 0x%04hx\n", line[0], line[0]); */

    // Comma
    if (line[0] == ',') {
        token.kind = Token::COMMA;
        ++line;
        return ERR_OK;
    }

    // String literal
    if (line[0] == '"') {
        ++line;
        token.kind = Token::STRING;
        token.value.string.pointer = line;
        for (; line[0] != '"'; ++line) {
            if (line[0] == '\n' || line[0] == '\0')
                return ERR_ASM_UNTERMINATED_STRING;
        }
        token.value.string.length = line - token.value.string.pointer;
        ++line;  // Account for closing quote
        /* _print_string_slice(token.value.literal_string); */
        return ERR_OK;
    }

    // Register
    // Case-insensitive
    if ((line[0] == 'R' || line[0] == 'r') &&
        (isdigit(line[1]) && !is_char_valid_in_identifier(line[2]))) {
        ++line;
        token.kind = Token::REGISTER;
        token.value.register_ = line[0] - '0';
        ++line;
        return ERR_OK;
    }

    // Directive
    // Case-insensitive
    if (line[0] == '.') {
        ++line;
        StringSlice directive;
        directive.pointer = line;
        while (is_char_valid_in_identifier(tolower(line[0]))) {
            ++line;
        }
        directive.length = line - directive.pointer;
        // Sets kind and value
        return directive_from_string(token, directive);
    }

    // Hex literal
    RETURN_IF_ERR(take_integer_hex(line, token));
    if (token.kind != Token::NONE) return ERR_OK;  // Tried to parse, but failed
    // Decimal literal
    RETURN_IF_ERR(take_integer_decimal(line, token));
    if (token.kind != Token::NONE) return ERR_OK;  // Tried to parse, but failed

    // Character cannot start an identifier -> invalid
    if (!is_char_valid_identifier_start(line[0])) {
        return ERR_ASM_INVALID_TOKEN;
    }

    // TODO: First character is already checked.

    // Label or instruction
    // Case-insensitive
    StringSlice identifier;
    identifier.pointer = line;
    ++line;
    while (is_char_valid_in_identifier(tolower(line[0]))) ++line;
    identifier.length = line - identifier.pointer;

    /* printf("IDENT: <"); */
    /* _print_string_slice(identifier); */
    /* printf(">\n"); */

    // Sets kind and value, if is valid instruction
    if (!try_instruction_from_string_slice(token, identifier)) {
        // Label
        // TODO: Check if length > MAX_LABEL
        token.kind = Token::LABEL;
        token.value.label = identifier;
    }
    return ERR_OK;
}

Error take_integer_hex(const char *&line, Token &token) {
    const char *new_line = line;

    bool negative = false;
    if (new_line[0] == '-') {
        ++new_line;
        negative = true;
    }
    // Only allow one 0 in prefixx
    if (new_line[0] == '0') ++new_line;
    // Must have prefix
    if (new_line[0] != 'x' && new_line[0] != 'X') {
        return ERR_OK;
    }
    ++new_line;

    if (new_line[0] == '-') {
        ++new_line;
        // Don't allow `-x-`
        if (negative) {
            return ERR_ASM_INVALID_TOKEN;
        }
        negative = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1])) ++new_line;

    // Not an integer
    // Continue to next token
    if (parse_hex_digit(new_line[0]) < 0) {
        return ERR_OK;
    }

    line = new_line;  // Skip [x0-] which was just checked
    token.kind = Token::INTEGER;

    Word number = 0;
    while (true) {
        const char ch = line[0];
        const int8_t digit = parse_hex_digit(ch);  // Catches '\0'
        if (digit < 0) {
            // Integer-suffix type situation
            if (ch != '\0' && is_char_valid_in_identifier(ch))
                return ERR_ASM_INVALID_TOKEN;
            break;
        }
        number <<= 4;
        number += digit;
        ++line;
    }

    // TODO: Maybe don't set sign here...
    number *= negative ? -1 : 1;
    token.value.integer = number;

    return ERR_OK;
}

Error take_integer_decimal(const char *&line, Token &token) {
    const char *new_line = line;

    bool negative = false;
    if (new_line[0] == '-') {
        ++new_line;
        negative = true;
    }
    // Don't allow any 0's before prefix
    if (new_line[0] == '#') ++new_line;  // Optional
    if (new_line[0] == '-') {
        ++new_line;
        // Don't allow `-#-`
        if (negative) {
            return ERR_ASM_INVALID_TOKEN;
        }
        negative = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1])) ++new_line;

    // Not an integer
    // Continue to next token
    if (!isdigit(new_line[0])) {
        return ERR_OK;
    }

    line = new_line;  // Skip [#0-] which was just checked
    token.kind = Token::INTEGER;

    Word number = 0;
    while (true) {
        const char ch = line[0];
        if (!isdigit(line[0])) {  // Catches '\0'
            // Integer-suffix type situation
            if (ch != '\0' && is_char_valid_in_identifier(ch))
                return ERR_ASM_INVALID_TOKEN;
            break;
        }
        number *= 10;
        number += ch - '0';
        ++line;
    }

    // TODO: Maybe don't set sign here...
    number *= negative ? -1 : 1;
    token.value.integer = number;

    return ERR_OK;
}

// Returns -1 if not a valid hex digit
int8_t parse_hex_digit(const char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
    return -1;
}

// TODO: Maybe return character, and return some sentinel value if invalid ?
Error escape_character(char *const ch) {
    switch (*ch) {
        case 'n':
            *ch = '\n';
            break;
        case 'r':
            *ch = '\r';
            break;
        case 't':
            *ch = '\t';
            break;
        case '0':
            *ch = '\0';
            break;
        default:
            return ERR_ASM_INVALID_ESCAPE_CHAR;
    }
    return ERR_OK;
}

bool is_char_eol(const char ch) {
    // EOF, EOL, or comment
    return ch == '\0' || ch == '\r' || ch == '\n' || ch == ';';
}
bool is_char_valid_in_identifier(const char ch) {
    // TODO: Perhaps `-` and other characters might be allowed ?
    return ch == '_' || isalpha(ch) || isdigit(ch);
}
bool is_char_valid_identifier_start(const char ch) {
    // TODO: Perhaps `-` and other characters might be allowed ?
    return ch == '_' || isalpha(ch);
}

bool string_equals_slice(const char *const target,
                         const StringSlice candidate) {
    // Equality will check \0-mismatch, so no worry of reading past target
    for (size_t i = 0; i < candidate.length; ++i) {
        if (tolower(candidate.pointer[i]) != tolower(target[i])) return false;
    }
    return true;
}

void copy_string_slice_to_string(char *dest, const StringSlice src) {
    for (size_t i = 0; i < src.length; ++i) {
        dest[i] = src.pointer[i];
    }
    dest[src.length] = '\0';
}

void print_string_slice(FILE *const &file, const StringSlice &slice) {
    for (size_t i = 0; i < slice.length; ++i) {
        fprintf(file, "%c", slice.pointer[i]);
    }
}

// TODO: Maybe make a `directive_names` array and index with the directive value
//     as int ? -- to get names as static strings.
//     Same with instructions

static const char *directive_to_string(const Directive directive) {
    switch (directive) {
        case Directive::ORIG:
            return "ORIG";
        case Directive::END:
            return "END";
        case Directive::FILL:
            return "FILL";
        case Directive::BLKW:
            return "BLKW";
        case Directive::STRINGZ:
            return "STRINGZ";
    }
    UNREACHABLE();
}

// TODO: Accept `StringSlice` instead of `char *`
Error directive_from_string(Token &token, const StringSlice directive) {
    token.kind = Token::DIRECTIVE;
    if (string_equals_slice("orig", directive)) {
        token.value.directive = Directive::ORIG;
    } else if (string_equals_slice("end", directive)) {
        token.value.directive = Directive::END;
    } else if (string_equals_slice("fill", directive)) {
        token.value.directive = Directive::FILL;
    } else if (string_equals_slice("blkw", directive)) {
        token.value.directive = Directive::BLKW;
    } else if (string_equals_slice("stringz", directive)) {
        token.value.directive = Directive::STRINGZ;
    } else {
        return ERR_ASM_INVALID_DIRECTIVE;
    }
    return ERR_OK;
}

static const char *instruction_to_string(const Instruction instruction) {
    switch (instruction) {
        case Instruction::ADD:
            return "ADD";
        case Instruction::AND:
            return "AND";
        case Instruction::NOT:
            return "NOT";
        case Instruction::BR:
            return "BR";
        case Instruction::BRN:
            return "RN";
        case Instruction::BRZ:
            return "RZ";
        case Instruction::BRP:
            return "RP";
        case Instruction::BRNZ:
            return "RNZ";
        case Instruction::BRZP:
            return "RZP";
        case Instruction::BRNP:
            return "RNP";
        case Instruction::BRNZP:
            return "RNZP";
        case Instruction::JMP:
            return "JMP";
        case Instruction::RET:
            return "RET";
        case Instruction::JSR:
            return "JSR";
        case Instruction::JSRR:
            return "JSRR";
        case Instruction::LD:
            return "LD";
        case Instruction::ST:
            return "ST";
        case Instruction::LDI:
            return "LDI";
        case Instruction::STI:
            return "STI";
        case Instruction::LDR:
            return "LDR";
        case Instruction::STR:
            return "STR";
        case Instruction::LEA:
            return "LEA";
        case Instruction::TRAP:
            return "TRAP";
        case Instruction::GETC:
            return "GETC";
        case Instruction::OUT:
            return "OUT";
        case Instruction::PUTS:
            return "PUTS";
        case Instruction::IN:
            return "IN";
        case Instruction::PUTSP:
            return "PUTSP";
        case Instruction::HALT:
            return "HALT";
        case Instruction::RTI:
            return "RTI";
    }
    UNREACHABLE();
}

bool try_instruction_from_string_slice(Token &token,
                                       const StringSlice &instruction) {
    if (string_equals_slice("add", instruction)) {
        token.value.instruction = Instruction::ADD;
    } else if (string_equals_slice("and", instruction)) {
        token.value.instruction = Instruction::AND;
    } else if (string_equals_slice("not", instruction)) {
        token.value.instruction = Instruction::NOT;
    } else if (string_equals_slice("br", instruction)) {
        token.value.instruction = Instruction::BR;
    } else if (string_equals_slice("brn", instruction)) {
        token.value.instruction = Instruction::BRN;
    } else if (string_equals_slice("brz", instruction)) {
        token.value.instruction = Instruction::BRZ;
    } else if (string_equals_slice("brp", instruction)) {
        token.value.instruction = Instruction::BRP;
    } else if (string_equals_slice("brnz", instruction)) {
        token.value.instruction = Instruction::BRNZ;
    } else if (string_equals_slice("brzp", instruction)) {
        token.value.instruction = Instruction::BRZP;
    } else if (string_equals_slice("brnp", instruction)) {
        token.value.instruction = Instruction::BRNP;
    } else if (string_equals_slice("brnzp", instruction)) {
        token.value.instruction = Instruction::BRNZP;
    } else if (string_equals_slice("jmp", instruction)) {
        token.value.instruction = Instruction::JMP;
    } else if (string_equals_slice("ret", instruction)) {
        token.value.instruction = Instruction::RET;
    } else if (string_equals_slice("jsr", instruction)) {
        token.value.instruction = Instruction::JSR;
    } else if (string_equals_slice("jsrr", instruction)) {
        token.value.instruction = Instruction::JSRR;
    } else if (string_equals_slice("ld", instruction)) {
        token.value.instruction = Instruction::LD;
    } else if (string_equals_slice("st", instruction)) {
        token.value.instruction = Instruction::ST;
    } else if (string_equals_slice("ldi", instruction)) {
        token.value.instruction = Instruction::LDI;
    } else if (string_equals_slice("sti", instruction)) {
        token.value.instruction = Instruction::STI;
    } else if (string_equals_slice("ldr", instruction)) {
        token.value.instruction = Instruction::LDR;
    } else if (string_equals_slice("str", instruction)) {
        token.value.instruction = Instruction::STR;
    } else if (string_equals_slice("lea", instruction)) {
        token.value.instruction = Instruction::LEA;
    } else if (string_equals_slice("trap", instruction)) {
        token.value.instruction = Instruction::TRAP;
    } else if (string_equals_slice("getc", instruction)) {
        token.value.instruction = Instruction::GETC;
    } else if (string_equals_slice("out", instruction)) {
        token.value.instruction = Instruction::OUT;
    } else if (string_equals_slice("puts", instruction)) {
        token.value.instruction = Instruction::PUTS;
    } else if (string_equals_slice("in", instruction)) {
        token.value.instruction = Instruction::IN;
    } else if (string_equals_slice("putsp", instruction)) {
        token.value.instruction = Instruction::PUTSP;
    } else if (string_equals_slice("halt", instruction)) {
        token.value.instruction = Instruction::HALT;
    } else if (string_equals_slice("rti", instruction)) {
        token.value.instruction = Instruction::RTI;
    } else {
        return false;
    }
    token.kind = Token::INSTRUCTION;
    return true;
}

void _print_token(const Token &token) {
    printf("Token: ");
    switch (token.kind) {
        case Token::INSTRUCTION: {
            printf("Instruction: %s\n",
                   instruction_to_string(token.value.instruction));
        }; break;
        case Token::DIRECTIVE: {
            printf("Directive: %s\n",
                   directive_to_string(token.value.directive));
        }; break;
        case Token::REGISTER: {
            printf("Register: R%d\n", token.value.register_);
        }; break;
        case Token::INTEGER: {
            printf("Integer: 0x%04hx #%d\n", token.value.integer,
                   token.value.integer);
        }; break;
        case Token::STRING: {
            printf("String: <");
            print_string_slice(stdout, token.value.string);
            printf(">\n");
        }; break;
        case Token::LABEL: {
            printf("Label: <");
            print_string_slice(stdout, token.value.label);
            printf(">\n");
        }; break;
        case Token::COMMA: {
            printf("Comma\n");
        }; break;
        case Token::NONE: {
            printf("NONE!\n");
        }; break;
    }
}

#endif
