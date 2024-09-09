#ifndef ASSEMBLE_CPP
#define ASSEMBLE_CPP

#include <cctype>   // isspace
#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memcpy
#include <vector>   // std::vector

#include "bitmasks.hpp"
#include "error.hpp"
#include "types.hpp"

using std::vector;

#define MAX_LINE 64
#define MAX_IDENTIFIER 32
#define MAX_LITERAL_STRING 32  // TODO: I think this should be longer ?

#define EXPECT_NEXT_TOKEN(line_ptr, token)              \
    {                                                   \
        RETURN_IF_ERR(get_next_token(line_ptr, token)); \
        if (token.tag == Token::NONE) {                 \
            fprintf(stderr, "Expected operand\n");      \
            return ERR_ASM_EXPECTED_OPERAND;            \
        }                                               \
    }

#define EXPECT_TOKEN_IS_TAG(token, tag)           \
    {                                             \
        if (token.tag != Token::tag) {            \
            fprintf(stderr, "Invalid operand\n"); \
            return ERR_ASM_INVALID_OPERAND;       \
        }                                         \
    }

#define EXPECT_NEXT_COMMA(line_ptr)                                \
    {                                                              \
        Token token;                                               \
        EXPECT_NEXT_TOKEN(line_ptr, token);                        \
        if (token.tag != Token::PUNCTUATION ||                     \
            token.value.punctuation != ',') {                      \
            fprintf(stderr, "Expected comma following operand\n"); \
            return ERR_ASM_EXPECTED_COMMA;                         \
        }                                                          \
    }

typedef char TokenStr[MAX_IDENTIFIER + 1];
typedef TokenStr LabelName;

// TODO: Merge label types
// TODO: Use a hashmap
typedef struct LabelDefinition {
    LabelName name;
    size_t index;
} LabelDefinition;

// Different type to label definition for clarity
typedef struct LabelReference {
    LabelName name;
    size_t index;
} LabelReference;

typedef enum Directive {
    ORIG,
    END,
    STRINGZ,
    FILL,
    BLKW,
} Directive;

typedef enum Instruction {
    ADD,
    AND,
    LEA,
    PUTS,
    HALT,
} Instruction;

typedef struct Token {
    enum {
        DIRECTIVE,
        INSTRUCTION,
        REGISTER,
        LABEL,
        LITERAL_STRING,
        LITERAL_INTEGER,
        PUNCTUATION,
        NONE,
    } tag;
    union {
        Directive directive;
        Instruction instruction;
        Register register_;
        TokenStr label;           // TODO: This might be able to be a pointer?
        TokenStr literal_string;  // TODO: This might be able to be a pointer?
        SignedWord literal_integer;
        char punctuation;
    } value;
} Token;

typedef uint8_t OpcodeValue;  // 4 bits

Error assemble(const char *const asm_filename, const char *const obj_filename);
Error read_asm_file_to_lines(const char *const filename, vector<Word> &words);
Error write_obj_file(const char *const filename, const vector<Word> &words);
Error get_next_token(const char *&line, Token &token);
// TODO: Add other prototypes

static const char *directive_to_string(Directive directive);
static const char *instruction_to_string(Instruction instruction);

bool find_label_definition(const TokenStr &needle,
                           const vector<LabelDefinition> &definitions,
                           size_t &index) {
    for (size_t j = 0; j < definitions.size(); ++j) {
        if (!strcmp(definitions[j].name, needle)) {
            index = definitions[j].index;
            return true;
        }
    }
    return false;
}

void _print_token(const Token &token);

Error assemble(const char *const asm_filename, const char *const obj_filename) {
    vector<Word> out_words;
    RETURN_IF_ERR(read_asm_file_to_lines(asm_filename, out_words));

    /* printf("Words: %ld\n", out_words.size()); */

    for (size_t i = 0; i < out_words.size(); ++i) {
        if (i > 0 && i % 8 == 0) {
            printf("\n");
        }
        printf("%04x ", out_words[i]);
    }
    printf("\n");

    RETURN_IF_ERR(write_obj_file(obj_filename, out_words));

    return ERR_OK;
}

Error write_obj_file(const char *const filename, const vector<Word> &words) {
    FILE *obj_file = fopen(filename, "wb");
    if (obj_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    for (size_t i = 0; i < words.size(); ++i) {
        Word word = swap_endian(words[i]);
        fwrite(&word, sizeof(Word), 1, obj_file);
        if (ferror(obj_file)) return ERR_FILE_WRITE;
    }

    fclose(obj_file);
    return ERR_OK;
}

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

Error read_asm_file_to_lines(const char *const filename, vector<Word> &words) {
    FILE *asm_file = fopen(filename, "r");
    if (asm_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    vector<LabelDefinition> label_definitions;
    vector<LabelReference> label_references;

    while (true) {
        /* printf("----------------\n"); */
        char line_buf[MAX_LINE];
        const char *line_ptr = line_buf;  // Pointer address is mutated
        if (fgets(line_buf, MAX_LINE, asm_file) == NULL) break;
        if (ferror(asm_file)) return ERR_FILE_READ;

        /* printf("<%s>", line_ptr); */

        Token token;
        RETURN_IF_ERR(get_next_token(line_ptr, token));
        /* _print_token(token); */

        // Empty line
        if (token.tag == Token::NONE) continue;

        if (words.size() == 0) {
            if (token.tag != Token::DIRECTIVE) {
                fprintf(stderr, "First line must be `.ORIG`\n");
                return ERR_ASM_EXPECTED_ORIG;
            }
            RETURN_IF_ERR(get_next_token(line_ptr, token));
            if (token.tag != Token::LITERAL_INTEGER) {
                fprintf(stderr, "Integer literal required after `.ORIG`\n");
                return ERR_ASM_EXPECTED_OPERAND;
            }
            words.push_back(token.value.literal_integer);
            continue;
        }

        if (token.tag == Token::DIRECTIVE) {
            switch (token.value.directive) {
                case Directive::END:
                    goto stop_parsing;

                case Directive::STRINGZ: {
                    // TODO: Escape characters
                    /* printf("%s\n", line_ptr); */
                    RETURN_IF_ERR(get_next_token(line_ptr, token));
                    /* _print_token(token); */
                    if (token.tag != Token::LITERAL_STRING) {
                        fprintf(stderr,
                                "String literal required after `.STRINGZ`\n");
                        return ERR_ASM_EXPECTED_OPERAND;
                    }
                    const char *string = token.value.literal_string;
                    for (char ch; (ch = string[0]) != '\0'; ++string) {
                        if (ch == '\\') {
                            ++string;
                            // "... \" is treated as unterminated
                            if (string[0] == '\0')
                                return ERR_ASM_UNTERMINATED_STRING;
                            ch = string[0];
                            RETURN_IF_ERR(escape_character(&ch));
                        }
                        words.push_back(static_cast<Word>(ch));
                    }
                    words.push_back(0x0000);  // Null-termination
                }; break;

                default:
                    // Includes `ORIG`
                    fprintf(stderr, "Unexpected directive `%s`\n",
                            directive_to_string(token.value.directive));
                    return ERR_ASM_UNEXPECTED_DIRECTIVE;
            }

            RETURN_IF_ERR(get_next_token(line_ptr, token));
            if (token.tag != Token::NONE) {
                fprintf(stderr, "Unexpected operand after directive\n");
                return ERR_ASM_UNEXPECTED_OPERAND;
            }
            continue;
        }

        if (token.tag == Token::LABEL) {
            TokenStr &name = token.value.label;
            for (size_t i = 0; i < label_definitions.size(); ++i) {
                if (!strcmp(label_definitions[i].name, name)) {
                    fprintf(stderr, "Duplicate label '%s'\n", name);
                    return ERR_ASM_DUPLICATE_LABEL;
                }
            }
            label_definitions.push_back({});
            memcpy(label_definitions.back().name, name,
                   sizeof(LabelDefinition));
            label_definitions.back().index = words.size();
            RETURN_IF_ERR(get_next_token(line_ptr, token));
        }

        // Line with only label
        if (token.tag == Token::NONE) continue;

        if (token.tag != Token::INSTRUCTION) {
            fprintf(stderr, "Expected instruction or end of line\n");
            return ERR_ASM_EXPECTED_INSTRUCTION;
        }

        Instruction instruction = token.value.instruction;
        printf("INSTRUCTION: %s\n", instruction_to_string(instruction));

        OpcodeValue opcode = 0;
        Word operands = 0;

        switch (instruction) {
            case Instruction::ADD:
            case Instruction::AND: {
                opcode =
                    instruction == Instruction::ADD ? OPCODE_ADD : OPCODE_AND;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                Register src_reg_a = token.value.register_;
                operands |= dest_reg << 6;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                if (token.tag == Token::REGISTER) {
                    Register src_reg_b = token.value.register_;
                    operands |= src_reg_b;
                } else if (token.tag == Token::LITERAL_INTEGER) {
                    SignedWord immediate = token.value.literal_integer;
                    if (immediate < -0b10000 || immediate > 0b11111) {
                        fprintf(stderr, "Immediate too large\n");
                        return ERR_ASM_IMMEDIATE_TOO_LARGE;
                    }
                    operands |= 1 << 5;  // Flag
                    operands |= immediate & BITMASK_LOW_5;
                }
            }; break;

            case Instruction::LEA: {
                opcode = OPCODE_LEA;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, LABEL);

                label_references.push_back({});
                memcpy(label_references.back().name, token.value.label,
                       sizeof(LabelReference));
                label_references.back().index = words.size();
            }; break;

            case Instruction::PUTS: {
                opcode = OPCODE_TRAP;
                operands = TRAP_PUTS;
            }; break;

            case Instruction::HALT: {
                opcode = OPCODE_TRAP;
                operands = TRAP_HALT;
            }; break;

            default:
                return ERR_UNIMPLEMENTED;
        }

        RETURN_IF_ERR(get_next_token(line_ptr, token));
        if (token.tag != Token::NONE) {
            fprintf(stderr, "Unexpected operand after instruction\n");
            return ERR_ASM_UNEXPECTED_OPERAND;
        }

        Word word = opcode << 12 | operands;
        words.push_back(word);
    }
stop_parsing:

    // Replace label references with PC offsets based on label definitions
    for (size_t i = 0; i < label_references.size(); ++i) {
        LabelReference &ref = label_references[i];
        /* printf("Resolving '%s' at 0x%04lx\n", ref.name, ref.index); */

        size_t index;
        if (!find_label_definition(ref.name, label_definitions, index)) {
            fprintf(stderr, "Undefined label '%s'\n", ref.name);
            return ERR_ASM_UNDEFINED_LABEL;
        }
        /* printf("Found definition at 0x%04lx\n", index); */

        size_t pc_offset = index - ref.index - 1;
        /* printf("PC offset: 0x%04lx\n", pc_offset); */

        words[ref.index] |= pc_offset & BITMASK_LOW_9;
    }

    fclose(asm_file);
    return ERR_OK;
}

#define EXPECT_CHAR_RETURN_ERR(ch, file)             \
    {                                                \
        const Error error = read_char((ch), (file)); \
        if (error != ERR_OK) return error;           \
    }

bool char_can_be_in_identifier(const char ch) {
    // TODO: Perhaps `-` and other characters might be allowed ?
    return ch == '_' || isalpha(ch) || isdigit(ch);
}
bool char_can_be_identifier_start(const char ch) {
    // TODO: Perhaps `-` and other characters might be allowed ?
    return ch == '_' || isalpha(ch);
}

bool char_is_eol(const char ch) {
    return ch == '\0' || ch == '\n' || ch == ';';
}

int8_t parse_hex_digit(const char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'Z') return ch - 'A';
    if (ch >= 'a' && ch <= 'z') return ch - 'a';
    return -1;
}

// Candidate is NOT null-terminated
// Match is CASE-INSENSITIVE
bool string_equals(const char *const candidate, const char *const target,
                   const size_t candidate_len) {
    for (size_t i = 0; i < candidate_len; ++i) {
        if (tolower(candidate[i]) != tolower(target[i])) return false;
    }
    return true;
}

static const char *directive_to_string(Directive directive) {
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
    unreachable();
}

Error directive_from_string(Token &token, const char *const directive,
                            size_t len) {
    if (string_equals(directive, "orig", len)) {
        token.value.directive = Directive::ORIG;
    } else if (string_equals(directive, "end", len)) {
        token.value.directive = Directive::END;
    } else if (string_equals(directive, "fill", len)) {
        token.value.directive = Directive::FILL;
    } else if (string_equals(directive, "blkw", len)) {
        token.value.directive = Directive::BLKW;
    } else if (string_equals(directive, "stringz", len)) {
        token.value.directive = Directive::STRINGZ;
    } else {
        return ERR_ASM_INVALID_DIRECTIVE;
    }
    return ERR_OK;
}

static const char *instruction_to_string(Instruction instruction) {
    switch (instruction) {
        case Instruction::ADD:
            return "ADD";
        case Instruction::AND:
            return "AND";
        case Instruction::LEA:
            return "LEA";
        case Instruction::PUTS:
            return "PUTS";
        case Instruction::HALT:
            return "HALT";
    }
    unreachable();
}

bool instruction_from_string(Token &token, const char *const instruction,
                             size_t len) {
    if (string_equals(instruction, "add", len)) {
        token.value.instruction = Instruction::ADD;
    } else if (string_equals(instruction, "and", len)) {
        token.value.instruction = Instruction::AND;
    } else if (string_equals(instruction, "lea", len)) {
        token.value.instruction = Instruction::LEA;
    } else if (string_equals(instruction, "puts", len)) {
        token.value.instruction = Instruction::PUTS;
    } else if (string_equals(instruction, "halt", len)) {
        token.value.instruction = Instruction::HALT;
    } else {
        return false;
    }
    return true;
}

Error get_next_token(const char *&line, Token &token) {
    token.tag = Token::NONE;

    // Ignore leading spaces
    while (isspace(line[0])) ++line;
    // Linebreak, EOF, or comment
    if (char_is_eol(line[0])) return ERR_OK;

    /* printf("<%c> 0x%04hx\n", line[0], line[0]); */

    // Comma
    if (line[0] == ',') {
        token.tag = Token::PUNCTUATION;
        token.value.punctuation = ',';
        ++line;
        return ERR_OK;
    }

    // String literal
    if (line[0] == '"') {
        ++line;
        token.tag = Token::LITERAL_STRING;
        size_t i = 0;
        for (; i < MAX_LITERAL_STRING; ++i) {
            if (line[0] == '\n' || line[0] == '\0')
                return ERR_ASM_UNTERMINATED_STRING;
            if (line[0] == '"') {
                ++line;
                break;
            }
            token.value.literal_string[i] = line[0];
            ++line;
        }
        token.value.literal_string[i] = '\0';
        return ERR_OK;
    }

    // Hex literal
    // TODO: Write these if statements better !
    if (line[0] == '0' && line[1] == 'x') {
        ++line;
    }
    if (line[0] == 'x') {
        token.tag = Token::LITERAL_INTEGER;
        ++line;
        Word number = 0;
        for (size_t i = 0;; ++i) {
            char ch = line[0];
            int8_t digit = parse_hex_digit(ch);
            if (digit < 0) {
                if (char_can_be_in_identifier(ch)) return ERR_ASM_INVALID_TOKEN;
                break;
            }
            number <<= 4;
            number += digit;
            ++line;
        }
        token.value.literal_integer = number;
        return ERR_OK;
    }

    // Decimal literal
    if (line[0] == '#' && (line[1] == '-' || isdigit(line[1]))) {
        ++line;
    }
    /* printf("%s\n", line); */
    if (line[0] == '-' || isdigit(line[0])) {
        token.tag = Token::LITERAL_INTEGER;
        Word sign = 1;
        if (line[0] == '-') {
            sign = -1;
            ++line;
        }
        Word number = 0;
        for (size_t i = 0;; ++i) {
            char ch = line[0];
            if (!isdigit(line[0])) {
                if (char_can_be_in_identifier(ch)) return ERR_ASM_INVALID_TOKEN;
                break;
            }
            number *= 10;
            number += ch - '0';
            ++line;
        }
        number *= sign;
        token.value.literal_integer = number;
        return ERR_OK;
    }

    // Register
    // Case-insensitive
    if (line[0] == 'R' || line[0] == 'r' && isdigit(line[1]) &&
                              !char_can_be_in_identifier(line[2])) {
        ++line;
        token.tag = Token::REGISTER;
        token.value.register_ = line[0] - '0';
        ++line;
        return ERR_OK;
    }

    // Directive
    // Case-insensitive
    if (line[0] == '.') {
        ++line;
        token.tag = Token::DIRECTIVE;
        const char *directive = line;
        size_t len = 0;
        for (; len < MAX_IDENTIFIER; ++len) {
            if (!char_can_be_in_identifier(tolower(line[0]))) break;
            ++line;
        }
        return directive_from_string(token, directive, len);
    }

    // Character cannot start an identifier -> invalid
    if (!char_can_be_identifier_start(line[0])) {
        return ERR_ASM_INVALID_TOKEN;
    }

    // Label or instruction
    // Case-insensitive
    const char *identifier = line;
    ++line;
    size_t len = 1;
    for (; len < MAX_IDENTIFIER; ++len) {
        if (!char_can_be_in_identifier(tolower(line[0]))) break;
        ++line;
    }

    if (instruction_from_string(token, identifier, len)) {
        // Instruction -- value already set
        token.tag = Token::INSTRUCTION;
    } else {
        // Label
        token.tag = Token::LABEL;
        size_t i = 0;
        for (; i < len; ++i) {
            token.value.label[i] = identifier[i];
        }
        token.value.label[i] = '\0';
        // TODO: Check if next character is colon !
    }
    return ERR_OK;
}

void _print_token(const Token &token) {
    printf("TOKEN: ");
    switch (token.tag) {
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
        case Token::LABEL: {
            printf("Label: <%s>\n", token.value.label);
        }; break;
        case Token::LITERAL_STRING: {
            printf("String: <%s>\n", token.value.literal_string);
        }; break;
        case Token::LITERAL_INTEGER: {
            printf("Integer: 0x%04hx #%d\n", token.value.literal_integer,
                   token.value.literal_integer);
        }; break;
        case Token::PUNCTUATION: {
            printf("Punctuation: <%c>\n", token.value.punctuation);
        }; break;
        case Token::NONE: {
            printf("NONE!\n");
        }; break;
    }
}

#endif
