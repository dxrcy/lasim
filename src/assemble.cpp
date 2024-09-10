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

#define MAX_LINE 512  // This can be large as it is not aggregated
#define MAX_LABEL 32

// TODO: Rename these macros probably
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
        RETURN_IF_ERR(get_next_token(line_ptr, token));            \
        if (token.tag != Token::COMMA) {                           \
            fprintf(stderr, "Expected comma following operand\n"); \
            return ERR_ASM_EXPECTED_COMMA;                         \
        }                                                          \
    }

#define EXPECT_EOL(line_ptr)                                           \
    {                                                                  \
        Token token;                                                   \
        RETURN_IF_ERR(get_next_token(line_ptr, token));                \
        if (token.tag != Token::NONE) {                                \
            fprintf(stderr, "Unexpected operand after instruction\n"); \
            return ERR_ASM_UNEXPECTED_OPERAND;                         \
        }                                                              \
    }

typedef struct StringSlice {
    const char *pointer;
    size_t length;
} StringSlice;

typedef char LabelString[MAX_LABEL];

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
        LABEL,
        LITERAL_STRING,
        LITERAL_INTEGER,
        COMMA,
        NONE,
    } tag;
    union {
        Directive directive;
        Instruction instruction;
        Register register_;
        StringSlice label;  // Gets copied on push to a labels vector
        StringSlice literal_string;
        SignedWord literal_integer;
    } value;
} Token;

typedef uint8_t OpcodeValue;  // 4 bits

Error assemble(const char *const asm_filename, const char *const obj_filename);
Error read_and_assemble(const char *const filename, vector<Word> &words);
Error write_obj_file(const char *const filename, const vector<Word> &words);
Error get_next_token(const char *&line, Token &token);
// TODO: Add other prototypes

static const char *directive_to_string(Directive directive);
static const char *instruction_to_string(Instruction instruction);

bool string_equals(const char *const candidate, const char *const target);

void _print_string_slice(const StringSlice &slice);
void _print_token(const Token &token);

bool find_label_definition(const LabelString &target,
                           const vector<LabelDefinition> &definitions,
                           SignedWord &index) {
    for (size_t j = 0; j < definitions.size(); ++j) {
        LabelDefinition candidate = definitions[j];
        if (string_equals(candidate.name, target)) {
            index = candidate.index;
            return true;
        }
    }
    return false;
}

void add_label_reference(vector<LabelReference> &references,
                         const StringSlice &name, const Word index,
                         const bool is_offset11) {
    references.push_back({});
    LabelReference &ref = references.back();
    memcpy(ref.name, name.pointer, name.length);
    ref.index = index;
    ref.is_offset11 = is_offset11;
    /* printf("label:<"); */
    /* _print_string_slice(name); */
    /* printf(">\n"); */
}

Error assemble(const char *const asm_filename, const char *const obj_filename) {
    vector<Word> out_words;
    RETURN_IF_ERR(read_and_assemble(asm_filename, out_words));

    RETURN_IF_ERR(write_obj_file(obj_filename, out_words));

    return ERR_OK;
}

Error write_obj_file(const char *const filename, const vector<Word> &words) {
    FILE *obj_file = fopen(filename, "wb");
    if (obj_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    /* printf("Size: %lu words\n", words.size()); */

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

// Must ONLY be called with a BR* instruction
ConditionCode branch_condition_code(Instruction instruction) {
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

Error read_and_assemble(const char *const filename, vector<Word> &words) {
    FILE *asm_file = fopen(filename, "r");
    if (asm_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    vector<LabelDefinition> label_definitions;
    vector<LabelReference> label_references;

    while (true) {
        char line_buf[MAX_LINE];
        const char *line_ptr = line_buf;  // Pointer address is mutated
        if (fgets(line_buf, MAX_LINE, asm_file) == NULL) break;
        if (ferror(asm_file)) return ERR_FILE_READ;

        /* printf("<%s>\n", line_ptr); */

        Token token;
        RETURN_IF_ERR(get_next_token(line_ptr, token));

        /* _print_token(token); */

        // Empty line
        if (token.tag == Token::NONE) continue;

        if (words.size() == 0) {
            /* _print_token(token); */
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

        // TODO: Parsing label before directive *might* have bad effects,
        //     although I cannot think of any off the top of my head.
        if (token.tag == Token::LABEL) {
            const StringSlice &name = token.value.label;
            for (size_t i = 0; i < label_definitions.size(); ++i) {
                if (!string_equals(label_definitions[i].name, name.pointer)) {
                    fprintf(stderr, "Duplicate label '%s'\n", name.pointer);
                    return ERR_ASM_DUPLICATE_LABEL;
                }
            }
            label_definitions.push_back({});
            LabelDefinition &def = label_definitions.back();
            memcpy(def.name, name.pointer, name.length);
            def.index = words.size();

            /* _print_string_slice(name); */
            RETURN_IF_ERR(get_next_token(line_ptr, token));
        }

        /* _print_token(token); */

        if (token.tag == Token::DIRECTIVE) {
            switch (token.value.directive) {
                case Directive::END:
                    goto stop_parsing;

                case Directive::STRINGZ: {
                    /* printf("%s\n", line_ptr); */
                    RETURN_IF_ERR(get_next_token(line_ptr, token));
                    /* _print_token(token); */
                    if (token.tag != Token::LITERAL_STRING) {
                        fprintf(stderr,
                                "String literal required after `.STRINGZ`\n");
                        return ERR_ASM_EXPECTED_OPERAND;
                    }
                    const char *string = token.value.literal_string.pointer;
                    /* printf("("); */
                    /* _print_string_slice(token.value.literal_string); */
                    /* printf(")\n"); */
                    for (size_t i = 0; i < token.value.literal_string.length;
                         ++i) {
                        char ch = string[i];
                        if (ch == '\\') {
                            ++i;
                            // "... \" is treated as unterminated
                            if (i > token.value.literal_string.length)
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
                    EXPECT_TOKEN_IS_TAG(token, LITERAL_INTEGER);
                    words.push_back(token.value.literal_integer);
                }; break;

                case Directive::BLKW: {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_TAG(token, LITERAL_INTEGER);
                    // TODO: Replace with better vector function
                    for (Word i = 0; i < token.value.literal_integer; ++i) {
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
        if (token.tag == Token::NONE) continue;

        if (token.tag != Token::INSTRUCTION) {
            /* _print_token(token); */
            fprintf(stderr, "Expected instruction or end of line\n");
            return ERR_ASM_EXPECTED_INSTRUCTION;
        }

        const Instruction instruction = token.value.instruction;
        /* printf("Instruction: %s\n", instruction_to_string(instruction));
         */

        Opcode opcode;
        Word operands = 0;

        switch (instruction) {
            case Instruction::ADD:
            case Instruction::AND: {
                opcode =
                    instruction == Instruction::ADD ? Opcode::ADD : Opcode::AND;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register src_reg_a = token.value.register_;
                operands |= src_reg_a << 6;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                if (token.tag == Token::REGISTER) {
                    const Register src_reg_b = token.value.register_;
                    operands |= src_reg_b;
                } else if (token.tag == Token::LITERAL_INTEGER) {
                    const SignedWord immediate = token.value.literal_integer;
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
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
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
                    branch_condition_code(instruction);
                operands |= condition << 9;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, LABEL);
                add_label_reference(label_references, token.value.label,
                                    words.size(), false);
            }; break;

            case Instruction::JMP:
            case Instruction::RET: {
                opcode = Opcode::JMP_RET;

                Register addr_reg = 7;  // Default R7 for `RET`
                if (instruction == Instruction::JMP) {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_TAG(token, REGISTER);
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
                    EXPECT_TOKEN_IS_TAG(token, LABEL);
                    add_label_reference(label_references, token.value.label,
                                        words.size(), true);
                } else {
                    EXPECT_NEXT_TOKEN(line_ptr, token);
                    EXPECT_TOKEN_IS_TAG(token, REGISTER);
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
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register ds_reg = token.value.register_;
                operands |= ds_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, LABEL);
                add_label_reference(label_references, token.value.label,
                                    words.size(), false);
            }; break;

            case Instruction::LDR:
            case Instruction::STR: {
                opcode =
                    instruction == Instruction::LDR ? Opcode::LDR : Opcode::STR;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register ds_reg = token.value.register_;
                operands |= ds_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register base_reg = token.value.register_;
                operands |= base_reg << 6;
                EXPECT_NEXT_COMMA(line_ptr);

                // TODO: Check should be aware of sign; See `ADD` case
                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, LITERAL_INTEGER);
                const SignedWord immediate = token.value.literal_integer;
                if (immediate >> 6 != 0) {
                    fprintf(stderr, "Immediate too large\n");
                    return ERR_ASM_IMMEDIATE_TOO_LARGE;
                }
                operands |= immediate & BITMASK_LOW_6;
            }; break;

            case Instruction::LEA: {
                opcode = Opcode::LEA;

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, REGISTER);
                const Register dest_reg = token.value.register_;
                operands |= dest_reg << 9;
                EXPECT_NEXT_COMMA(line_ptr);

                EXPECT_NEXT_TOKEN(line_ptr, token);
                EXPECT_TOKEN_IS_TAG(token, LABEL);
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
                        EXPECT_TOKEN_IS_TAG(token, LITERAL_INTEGER);
                        const SignedWord immediate =
                            token.value.literal_integer;
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

        Word word = static_cast<Word>(opcode) << 12 | operands;
        words.push_back(word);
    }
stop_parsing:

    // Replace label references with PC offsets based on label definitions
    for (size_t i = 0; i < label_references.size(); ++i) {
        LabelReference &ref = label_references[i];
        /* printf("Resolving '%s' at 0x%04x\n", ref.name, ref.index); */

        SignedWord index;
        if (!find_label_definition(ref.name, label_definitions, index)) {
            fprintf(stderr, "Undefined label '%s'\n", ref.name);
            return ERR_ASM_UNDEFINED_LABEL;
        }
        /* printf("Found definition at 0x%04lx\n", index); */

        /* printf("0x%04x -> 0x%04x\n", ref.index, index); */
        SignedWord pc_offset = index - ref.index - 1;
        /* printf("PC offset: 0x%04lx\n", pc_offset); */

        Word mask = (1U << (ref.is_offset11 ? 11 : 9)) - 1;

        words[ref.index] |= pc_offset & mask;
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
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'z') return ch - 'a' + 10;
    return -1;
}

// TODO: Replace with standard lib function
// Candidate is NOT null-terminated
// Match is CASE-INSENSITIVE
bool string_equals(const char *candidate, const char *target) {
    // Equality will check \0-mismatch, so no worry of reading past string
    for (; candidate[0] != '\0'; ++candidate, ++target) {
        if (tolower(candidate[0]) != tolower(target[0])) return false;
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
    UNREACHABLE();
}

Error directive_from_string(Token &token, const char *const directive) {
    if (string_equals("orig", directive)) {
        token.value.directive = Directive::ORIG;
    } else if (string_equals("end", directive)) {
        token.value.directive = Directive::END;
    } else if (string_equals("fill", directive)) {
        token.value.directive = Directive::FILL;
    } else if (string_equals("blkw", directive)) {
        token.value.directive = Directive::BLKW;
    } else if (string_equals("stringz", directive)) {
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

bool instruction_from_string(Token &token, const char *const instruction) {
    if (string_equals("add", instruction)) {
        token.value.instruction = Instruction::ADD;
    } else if (string_equals("and", instruction)) {
        token.value.instruction = Instruction::AND;
    } else if (string_equals("not", instruction)) {
        token.value.instruction = Instruction::NOT;
    } else if (string_equals("br", instruction)) {
        token.value.instruction = Instruction::BR;
    } else if (string_equals("brn", instruction)) {
        token.value.instruction = Instruction::BRN;
    } else if (string_equals("brz", instruction)) {
        token.value.instruction = Instruction::BRZ;
    } else if (string_equals("brp", instruction)) {
        token.value.instruction = Instruction::BRP;
    } else if (string_equals("brnz", instruction)) {
        token.value.instruction = Instruction::BRNZ;
    } else if (string_equals("brzp", instruction)) {
        token.value.instruction = Instruction::BRZP;
    } else if (string_equals("brnp", instruction)) {
        token.value.instruction = Instruction::BRNP;
    } else if (string_equals("brnzp", instruction)) {
        token.value.instruction = Instruction::BRNZP;
    } else if (string_equals("jmp", instruction)) {
        token.value.instruction = Instruction::JMP;
    } else if (string_equals("ret", instruction)) {
        token.value.instruction = Instruction::RET;
    } else if (string_equals("jsr", instruction)) {
        token.value.instruction = Instruction::JSR;
    } else if (string_equals("jsrr", instruction)) {
        token.value.instruction = Instruction::JSRR;
    } else if (string_equals("ld", instruction)) {
        token.value.instruction = Instruction::LD;
    } else if (string_equals("st", instruction)) {
        token.value.instruction = Instruction::ST;
    } else if (string_equals("ldi", instruction)) {
        token.value.instruction = Instruction::LDI;
    } else if (string_equals("sti", instruction)) {
        token.value.instruction = Instruction::STI;
    } else if (string_equals("ldr", instruction)) {
        token.value.instruction = Instruction::LDR;
    } else if (string_equals("str", instruction)) {
        token.value.instruction = Instruction::STR;
    } else if (string_equals("lea", instruction)) {
        token.value.instruction = Instruction::LEA;
    } else if (string_equals("trap", instruction)) {
        token.value.instruction = Instruction::TRAP;
    } else if (string_equals("getc", instruction)) {
        token.value.instruction = Instruction::GETC;
    } else if (string_equals("out", instruction)) {
        token.value.instruction = Instruction::OUT;
    } else if (string_equals("puts", instruction)) {
        token.value.instruction = Instruction::PUTS;
    } else if (string_equals("in", instruction)) {
        token.value.instruction = Instruction::IN;
    } else if (string_equals("putsp", instruction)) {
        token.value.instruction = Instruction::PUTSP;
    } else if (string_equals("halt", instruction)) {
        token.value.instruction = Instruction::HALT;
    } else if (string_equals("rti", instruction)) {
        token.value.instruction = Instruction::RTI;
    } else {
        return false;
    }
    return true;
}

Error parse_literal_integer_hex(const char *&line, Token &token) {
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
    token.tag = Token::LITERAL_INTEGER;

    Word number = 0;
    while (true) {
        char ch = line[0];
        int8_t digit = parse_hex_digit(ch);  // Catches '\0'
        if (digit < 0) {
            // Integer-suffix type situation
            if (ch != '\0' && char_can_be_in_identifier(ch))
                return ERR_ASM_INVALID_TOKEN;
            break;
        }
        number <<= 4;
        number += digit;
        ++line;
    }

    // TODO: Maybe don't set sign here...
    number *= negative ? -1 : 1;
    token.value.literal_integer = number;

    return ERR_OK;
}

Error parse_literal_integer_decimal(const char *&line, Token &token) {
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
    token.tag = Token::LITERAL_INTEGER;

    Word number = 0;
    while (true) {
        char ch = line[0];
        if (!isdigit(line[0])) {  // Catches '\0'
            // Integer-suffix type situation
            if (ch != '\0' && char_can_be_in_identifier(ch))
                return ERR_ASM_INVALID_TOKEN;
            break;
        }
        number *= 10;
        number += ch - '0';
        ++line;
    }

    // TODO: Maybe don't set sign here...
    number *= negative ? -1 : 1;
    token.value.literal_integer = number;

    return ERR_OK;
}

Error get_next_token(const char *&line, Token &token) {
    token.tag = Token::NONE;

    /* printf("-->%c<\n", line[0]); */

    // Ignore leading spaces
    while (isspace(line[0])) ++line;
    // Linebreak, EOF, or comment
    if (char_is_eol(line[0])) return ERR_OK;

    /* printf("<<%s>>\n", line); */

    /* printf("<%c> 0x%04hx\n", line[0], line[0]); */

    // Comma
    if (line[0] == ',') {
        token.tag = Token::COMMA;
        ++line;
        return ERR_OK;
    }

    // String literal
    if (line[0] == '"') {
        ++line;
        token.tag = Token::LITERAL_STRING;
        token.value.literal_string.pointer = line;
        for (; line[0] != '"'; ++line) {
            if (line[0] == '\n' || line[0] == '\0')
                return ERR_ASM_UNTERMINATED_STRING;
        }
        token.value.literal_string.length =
            line - token.value.literal_string.pointer;
        ++line;  // Account for closing quote
        /* _print_string_slice(token.value.literal_string); */
        return ERR_OK;
    }

    // Register
    // Case-insensitive
    if ((line[0] == 'R' || line[0] == 'r') &&
        (isdigit(line[1]) && !char_can_be_in_identifier(line[2]))) {
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
        while (char_can_be_in_identifier(tolower(line[0]))) {
            ++line;
        }
        return directive_from_string(token, directive);
    }

    // Hex literal
    RETURN_IF_ERR(parse_literal_integer_hex(line, token));
    if (token.tag != Token::NONE) return ERR_OK;  // Tried to parse, but failed
    // Decimal literal
    RETURN_IF_ERR(parse_literal_integer_decimal(line, token));
    if (token.tag != Token::NONE) return ERR_OK;  // Tried to parse, but failed

    // Character cannot start an identifier -> invalid
    if (!char_can_be_identifier_start(line[0])) {
        return ERR_ASM_INVALID_TOKEN;
    }

    // Label or instruction
    // Case-insensitive
    StringSlice identifier;
    identifier.pointer = line;
    ++line;
    for (; char_can_be_in_identifier(tolower(line[0])); ++line) {
        if (!char_can_be_in_identifier(tolower(line[0]))) break;
    }
    identifier.length = line - identifier.pointer;

    /* printf("IDENT: <"); */
    /* _print_string_slice(identifier); */
    /* printf(">\n"); */

    if (instruction_from_string(token, identifier.pointer)) {
        // Instruction -- value already set
        token.tag = Token::INSTRUCTION;
    } else {
        // Label
        // TODO: Check if length > MAX_LABEL
        token.tag = Token::LABEL;
        token.value.label = identifier;
    }
    return ERR_OK;
}

void _print_string_slice(const StringSlice &slice) {
    for (size_t i = 0; i < slice.length; ++i) {
        printf("%c", slice.pointer[i]);
    }
}

void _print_token(const Token &token) {
    printf("Token: ");
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
            printf("Label: <");
            _print_string_slice(token.value.label);
            printf(">\n");
        }; break;
        case Token::LITERAL_STRING: {
            printf("String: <");
            _print_string_slice(token.value.literal_string);
            printf(">\n");
        }; break;
        case Token::LITERAL_INTEGER: {
            printf("Integer: 0x%04hx #%d\n", token.value.literal_integer,
                   token.value.literal_integer);
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
