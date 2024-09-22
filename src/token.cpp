#ifndef TOKEN_CPP
#define TOKEN_CPP

#include <cctype>   // isspace
#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // strcmp, strncmp

#include "error.hpp"
#include "slice.cpp"
#include "types.hpp"

#define MAX_LABEL 32  // Includes '\0'

#define RETURN_IF_FAILED(_failed) \
    if (_failed)                  \
        return;

// Must be a copied string, as `line` is overwritten
// Case is preserved, but must be ignoring when comparing labels
typedef char LabelString[MAX_LABEL];

typedef struct LabelDefinition {
    LabelString name;
    Word index;
} LabelDefinition;

typedef struct LabelReference {
    LabelString name;
    Word index;
    int line_number;   // For diagnostic
    bool is_offset11;  // Used for `JSR` only
} LabelReference;

enum class Directive {
    ORIG,
    END,
    FILL,
    BLKW,
    STRINGZ,
};

// TODO(feat/diagnostic): Warn on unused labels ?

// MUST match order of `Directive` enum
static const char *const DIRECTIVE_NAMES[] = {
    "ORIG",
    "END",
    "FILL",
    "BLKW",
    "STRINGZ",
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
    RTI   // Only used in 'supervisor' mode
};

// MUST match order of `Instruction` enum
// Note the case of BR* instructions
static const char *const INSTRUCTION_NAMES[] = {
    "ADD",  "AND",  "NOT",   "BR",    "BRn",  "BRz", "BRp",  "BRnz",
    "BRzp", "BRnp", "BRNzp", "JMP",   "RET",  "JSR", "JSRR", "LD",
    "ST",   "LDI",  "STI",   "LDR",   "STR",  "LEA", "TRAP", "GETC",
    "OUT",  "PUTS", "IN",    "PUTSP", "HALT", "REG", "RTI",
};

// Can be signed or unsigned
// Intended sign needs to be known to check if integer is too large for
//     a particular instruction
// Value is stored unsigned if `!is_signed` (high bit does not imply negative)
// Value is stored signed if `is_signed`, in 2's compliment, but stored in an
//     unsigned type
typedef struct InitialSignWord {
    Word value;
    bool is_signed;
} InitialSignWord;

enum class TokenKind {
    DIRECTIVE,
    INSTRUCTION,
    REGISTER,
    INTEGER,
    STRING,
    LABEL,
    COMMA,
    COLON,
    EOL,
};
typedef struct Token {
    TokenKind kind;
    union {
        Directive directive;
        Instruction instruction;
        Register register_;
        // Sign depends on if `-` character is present in asm file
        InitialSignWord integer;
        // `StringSlice`s are only valid for lifetime of `line`
        // Length should be checked on construction of token
        StringSlice string;  // Gets copied on push to words vector
        StringSlice label;   // Gets copied on push to a labels vector
    } value;
} Token;

// Note: 'take' here means increment the line pointer and return a token
void take_next_token(const char *&line, Token &token, bool &failed);
// Used by `take_next_token`
void take_literal_string(const char *&line, Token &token, bool &failed);
void take_directive(const char *&line, Token &token, bool &failed);
void take_register(const char *&line, Token &token);
void take_integer_token(const char *&line, Token &token, bool &failed);
int take_integer(const char *&line, InitialSignWord &number);
int take_integer_hex(const char *&line, InitialSignWord &number);
int take_integer_decimal(const char *&line, InitialSignWord &number);
int8_t parse_hex_digit(const char ch);
bool append_decimal_digit_checked(
    Word &number, uint8_t digit, bool is_negative
);
bool is_char_eol(const char ch);
bool is_char_valid_in_identifier(const char ch);
bool is_char_valid_identifier_start(const char ch);

void print_invalid_token(const char *const &line);

// Enums to/from string
static const char *directive_to_string(const Directive directive);
bool directive_from_string(Token &token, const StringSlice directive);
static const char *instruction_to_string(const Instruction instruction);
bool instruction_from_string_slice(
    Token &token, const StringSlice &instruction
);
static const char *token_kind_to_string(const TokenKind token_kind);

// Debugging
void _print_token(const Token &token);

void take_next_token(const char *&line, Token &token, bool &failed) {
    token.kind = TokenKind::EOL;

    // Ignore leading spaces
    while (isspace(line[0]))
        ++line;
    // Linebreak, EOF, or comment
    if (is_char_eol(line[0]))
        return;

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
    take_literal_string(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind != TokenKind::EOL)
        return;  // Tried to parse, but failed

    // Register
    take_register(line, token);  // Cannot fail
    RETURN_IF_FAILED(failed);

    // Directive
    take_directive(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind != TokenKind::EOL)
        return;  // Tried to parse, but failed

    // Hex/decimal literal
    take_integer_token(line, token, failed);
    RETURN_IF_FAILED(failed);
    if (token.kind != TokenKind::EOL)
        return;  // Tried to parse, but failed

    // Character cannot start an identifier -> invalid
    if (!is_char_valid_identifier_start(line[0])) {
        print_invalid_token(line);
        failed = true;
        return;
    }

    // Label or instruction
    StringSlice identifier;
    identifier.pointer = line;
    ++line;
    while (is_char_valid_in_identifier(tolower(line[0])))
        ++line;
    identifier.length = line - identifier.pointer;

    // Sets kind and value, if is valid instruction
    if (!instruction_from_string_slice(token, identifier)) {
        // Label
        if (identifier.length >= MAX_LABEL) {
            fprintf(stderr, "Label is over %d characters: `", MAX_LABEL);
            print_string_slice(stderr, identifier);
            fprintf(stderr, "`\n");
            failed = true;
            return;
        }
        token.kind = TokenKind::LABEL;
        token.value.label = identifier;
    }
}

void take_literal_string(const char *&line, Token &token, bool &failed) {
    if (line[0] != '"')
        return;
    ++line;  // Opening quote

    token.kind = TokenKind::STRING;
    token.value.string.pointer = line;
    for (; line[0] != '"'; ++line) {
        // String cannot be multi-line, or unclosed within a file
        if (line[0] == '\n' || line[0] == '\0') {
            fprintf(stderr, "Unterminated string literal\n");
            failed = true;
            return;
        }
    }

    token.value.string.length = line - token.value.string.pointer;
    ++line;  // Closing quote
}

void take_directive(const char *&line, Token &token, bool &failed) {
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
    if (!directive_from_string(token, directive)) {
        fprintf(stderr, "Invalid directive `.");
        print_string_slice(stderr, directive);
        fprintf(stderr, "`\n");
        failed = true;
    }
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

// TODO(refactor): Change `int` return to `ParseResult` enum
int take_integer_hex(const char *&line, InitialSignWord &integer) {
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
        return 0;
    }
    ++new_line;

    if (new_line[0] == '-') {
        ++new_line;
        // Don't allow `-x-`
        if (is_signed) {
            return -1;
        }
        is_signed = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1]))
        ++new_line;

    // Not an integer
    // Continue to next token
    if (parse_hex_digit(new_line[0]) < 0)
        return 0;

    line = new_line;  // Skip [x0-] which was just checked
    integer.is_signed = is_signed;

    Word number = 0;
    for (size_t i = 0;; ++i) {
        const char ch = line[0];
        const int8_t digit = parse_hex_digit(ch);  // Catches '\0'
        if (digit < 0) {
            // Checks if number is immediately followed by identifier character
            //     (like a suffix), which is invalid
            if (ch != '\0' && is_char_valid_in_identifier(ch)) {
                return -1;
            }
            break;
        }
        // Hex literals cannot be more than 4 digits
        // Leading zeros have already been skipped
        // Ignore sign
        if (i >= 4) {
            fprintf(stderr, "Integer literal is too large for a word\n");
            return -1;
        }
        number <<= 4;
        number += digit;
        ++line;
    }

    if (is_signed)
        number *= -1;  // Store negative number in unsigned word
    integer.value = number;
    return 1;
}

int take_integer_decimal(const char *&line, InitialSignWord &integer) {
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
            return -1;
        }
        is_signed = true;
    }
    while (new_line[0] == '0' && isdigit(new_line[1]))
        ++new_line;

    // Not an integer
    // Continue to next token
    if (!isdigit(new_line[0]))
        return 0;

    line = new_line;  // Skip [#0-] which was just checked
    integer.is_signed = is_signed;

    Word number = 0;
    while (true) {
        const char ch = line[0];
        if (!isdigit(line[0])) {  // Catches '\0'
            // Checks if number is immediately followed by identifier character
            //     (like a suffix), which is invalid
            if (ch != '\0' && is_char_valid_in_identifier(ch)) {
                return -1;
            }
            break;
        }
        if (!append_decimal_digit_checked(number, ch - '0', is_signed)) {
            fprintf(stderr, "Integer literal is too large for a word\n");
            return -1;
        }
        ++line;
    }

    if (is_signed)
        number *= -1;  // Store negative number in unsigned word
    integer.value = number;
    return 1;
}

int take_integer(const char *&line, InitialSignWord &integer) {
    int result;
    result = take_integer_hex(line, integer);
    if (result != 0)
        return result;
    result = take_integer_decimal(line, integer);
    if (result != 0)
        return result;
    return 0;
}

void take_integer_token(const char *&line, Token &token, bool &failed) {
    const char *const line_start = line;
    int result = take_integer(line, token.value.integer);
    if (result == -1) {
        print_invalid_token(line_start);
        failed = true;
        return;
    }
    if (result == 0)
        return;
    token.kind = TokenKind::INTEGER;
}

// Returns -1 if not a valid hex digit
// TODO(refactor): Don't use sentinel value for `parse_hex_digit`
int8_t parse_hex_digit(const char ch) {
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

bool append_decimal_digit_checked(
    Word &number, uint8_t digit, bool is_negative
) {
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

void print_invalid_token(const char *const &line) {
    fprintf(stderr, "Invalid token: `");
    fprintf(stderr, "%c", line[0]);
    // Print rest of instruction/label/integer if not starting with punctuation
    if (isalnum(line[0])) {
        for (size_t i = 1;; ++i) {
            const char ch = line[i];
            // Only these symbols can terminate a label
            if (isspace(ch) || ch == ',' || ch == ':')
                break;
            fprintf(stderr, "%c", ch);
        }
    }
    fprintf(stderr, "`\n");
}

static const char *directive_to_string(const Directive directive) {
    return DIRECTIVE_NAMES[static_cast<size_t>(directive)];
}
bool directive_from_string(Token &token, const StringSlice directive) {
    for (size_t i = 0; i < sizeof(DIRECTIVE_NAMES) / sizeof(char *); ++i) {
        if (string_equals_slice(DIRECTIVE_NAMES[i], directive)) {
            token.kind = TokenKind::DIRECTIVE;
            token.value.directive = static_cast<Directive>(i);
            return true;
        }
    }
    return false;
}

static const char *instruction_to_string(const Instruction instruction) {
    return INSTRUCTION_NAMES[static_cast<size_t>(instruction)];
}
bool instruction_from_string_slice(
    Token &token, const StringSlice &instruction
) {
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

static const char *token_kind_to_string(const TokenKind token_kind) {
    switch (token_kind) {
        case TokenKind::INSTRUCTION:
            return "instruction";
        case TokenKind::DIRECTIVE:
            return "directive";
        case TokenKind::REGISTER:
            return "register";
        case TokenKind::INTEGER:
            return "integer";
        case TokenKind::STRING:
            return "string";
        case TokenKind::LABEL:
            return "label";
        case TokenKind::COMMA:
            return "comma";
        case TokenKind::COLON:
            return "colon";
        case TokenKind::EOL:
            return "end of line";
    }
    UNREACHABLE();
}

void _print_token(const Token &token) {
    printf("Token: ");
    switch (token.kind) {
        case TokenKind::INSTRUCTION:
            printf(
                "Instruction: %s\n",
                instruction_to_string(token.value.instruction)
            );
            break;
        case TokenKind::DIRECTIVE:
            printf(
                "Directive: %s\n", directive_to_string(token.value.directive)
            );
            break;
        case TokenKind::REGISTER:
            printf("Register: R%d\n", token.value.register_);
            break;
        case TokenKind::INTEGER:
            if (token.value.integer.is_signed) {
                printf(
                    "Integer: 0x%04hx #%hd\n",
                    token.value.integer.value,
                    token.value.integer.value
                );
            } else {
                printf(
                    "Integer: 0x%04hx #+%hu\n",
                    token.value.integer.value,
                    token.value.integer.value
                );
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
        case TokenKind::EOL:
            printf("NONE!\n");
            break;
    }
}

#endif
