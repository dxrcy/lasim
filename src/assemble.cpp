#ifndef ASSEMBLE_CPP
#define ASSEMBLE_CPP

#include <cctype>  // isspace
#include <cstdio>  // FILE, fprintf, etc
#include <vector>  // std::vector

#include "error.hpp"
#include "types.hpp"

using std::vector;

#define IDENTIFIER_MAX_LENGTH 32
#define LITERAL_STRING_MAX_LENGTH 32  // TODO: I think this should be longer ?

typedef char TokenStr[IDENTIFIER_MAX_LENGTH + 1];
typedef TokenStr LabelName;

typedef struct LabelDef {
    LabelName name;
    size_t word_index;
} LabelDef;

typedef struct Token {
    enum {
        IDENTIFIER,
        DIRECTIVE,
        LITERAL_INTEGER,
        LITERAL_STRING,
        PUNCTUATION,
        NONE,
    } tag;
    union {
        TokenStr identifier;
        TokenStr directive;
        Word literal_integer;
        TokenStr literal_string;
        char punctuation;
    } value;
} Token;

Error assemble(const char *const asm_filename, const char *const obj_filename);
Error read_next_token(FILE *const asm_file, Token &token);

void _print_token(const Token &token);

Error assemble(const char *const asm_filename,
               const char *const _obj_filename) {
    FILE *asm_file = fopen(asm_filename, "r");
    if (asm_file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", asm_filename);
        return ERR_FILE_OPEN;
    }

    vector<LabelDef> label_definitions;

    while (true) {
        Token token;
        RETURN_IF_ERR(read_next_token(asm_file, token));
        _print_token(token);
        if (feof(asm_file)) break;
    }

    fclose(asm_file);
    return ERR_UNIMPLEMENTED;
}

#define EXPECT_CHAR_RETURN_ERR(ch, file)             \
    {                                                \
        const Error error = read_char((ch), (file)); \
        if (error != ERR_OK) return error;           \
    }

Error read_char(char &ch, FILE *const file) {
    int bytes_read = fread(&ch, sizeof(char), 1, file);
    if (feof(file)) return ERR_FILE_UNEXPECTED_EOF;
    if (ferror(file)) return ERR_FILE_READ;
    // Don't check for `bytes_read < 1` because already checked EOF
    if (bytes_read > 1) return ERR_FILE_TOO_LONG;
    if (!isascii(ch)) return ERR_FILE_NOT_ASCII;
    return ERR_OK;
}

Error read_next_real_char(char &ch, FILE *const file) {
    while (true) {
        EXPECT_CHAR_RETURN_ERR(ch, file);

        // On comma start, read to end of line
        // Treat character on next line as next character
        if (ch == ';') {
            // TODO: Maybe write as a do-while loop
            while (true) {
                EXPECT_CHAR_RETURN_ERR(ch, file);
                if (ch == '\n') {
                    break;
                }
            }
        }

        // Ignore whitespace, read next char
        if (isspace(ch)) continue;

        // Character must be non-whitespace and non-comment
        return ERR_OK;
    }
}

bool char_can_be_in_identifier(const char ch) {
    // TODO: Perhaps `-` and other characters might be allowed ?
    return ch == '_' || isalpha(ch) || isdigit(ch);
}

Error read_next_token(FILE *const file, Token &token) {
    token.tag = Token::NONE;

    char ch;
    read_next_real_char(ch, file);

    if (ch == '.') {
        token.tag = Token::DIRECTIVE;
        size_t i = 0;  // Don't include '.' in token

        for (;; ++i) {
            if (i >= IDENTIFIER_MAX_LENGTH) {
                return ERR_ASM_TOKEN_TOO_LONG;
            }
            EXPECT_CHAR_RETURN_ERR(ch, file);
            if (!char_can_be_in_identifier(ch)) break;
            token.value.directive[i] = ch;
        }
        token.value.directive[i] = '\0';
    } else if (ch == 'x') {
        // TODO
    } else if (ch == '#' || isdigit(ch)) {
        // TODO
    } else if (ch == '\'') {
        // TODO
    } else if (ch == '"') {
        token.tag = Token::LITERAL_STRING;
        size_t i = 0;  // Don't include opening '"'

        for (;; ++i) {
            if (i > LITERAL_STRING_MAX_LENGTH) {
                return ERR_ASM_TOKEN_TOO_LONG;
            }
            EXPECT_CHAR_RETURN_ERR(ch, file);
            if (ch == '"') break;
            token.value.literal_string[i] = ch;
        }
        token.value.literal_string[i] = '\0';
    } else {
        // Instruction name or label
        token.tag = Token::IDENTIFIER;
        token.value.identifier[0] = ch;
        size_t i = 1;  // Include first char

        for (;; ++i) {
            if (i >= IDENTIFIER_MAX_LENGTH) {
                return ERR_ASM_TOKEN_TOO_LONG;
            }
            EXPECT_CHAR_RETURN_ERR(ch, file);
            // TODO: This consumes the next character!
            //      This will be an issue, if 2 tokens appear next to directly
            //      each other, such as `R0,` [R0][,]
            // TODO: Seek back 1 byte in file
            if (!char_can_be_in_identifier(ch)) break;
            token.value.identifier[i] = ch;
        }
        token.value.identifier[i] = '\0';
        // TODO: If the next character is `:`, then skip it !!
    }

    return ERR_OK;
}

void _print_token(const Token &token) {
    printf("TOKEN: ");
    switch (token.tag) {
        case Token::IDENTIFIER: {
            printf("Identifier: <%s>\n", token.value.identifier);
        }; break;
        case Token::DIRECTIVE: {
            printf("Directive: <%s>\n", token.value.directive);
        }; break;
        case Token::LITERAL_STRING: {
            printf("String literal: <%s>\n", token.value.literal_string);
        }; break;
        case Token::LITERAL_INTEGER: {
            printf("Integer literal: 0x%04hx #%d\n",
                   token.value.literal_integer, token.value.literal_integer);
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
