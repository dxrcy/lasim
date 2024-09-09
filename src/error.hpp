#ifndef ERROR_HPP
#define ERROR_HPP

#define unreachable()                                          \
    {                                                          \
        fprintf(stderr, "Unreachable code reached. Uh oh!\n"); \
        exit(ERR_UNREACHABLE);                                 \
    }

// TODO: Maybe this can print some log in debug mode ?
#define IGNORE_ERR(result) result

#define RETURN_IF_ERR(result)                \
    {                                        \
        /* `input` may be a function call */ \
        const Error error = (result);        \
        if (error != ERR_OK) return error;   \
    }

typedef enum Error {
    // Ok
    ERR_OK = 0x00,
    // CLI arguments
    ERR_CLI = 0x10,
    // File operations
    ERR_FILE_OPEN = 0x21,
    ERR_FILE_READ = 0x22,
    ERR_FILE_WRITE = 0x23,
    ERR_FILE_TOO_SHORT = 0x24,       // No instructions
    ERR_FILE_TOO_LONG = 0x25,        // Too many instructions to fit in memory
    ERR_FILE_NOT_ASCII = 0x26,       // Character in ASM file is not valid ASCII
    ERR_FILE_UNEXPECTED_EOF = 0x26,  // File ends before a token is complete
    // Assembly parsing
    ERR_ASM_TOKEN_TOO_LONG = 0x31,
    ERR_ASM_UNTERMINATED_STRING = 0x32,
    ERR_ASM_INVALID_TOKEN = 0x33,
    ERR_ASM_INVALID_DIRECTIVE = 0x34,
    ERR_ASM_INVALID_REGISTER = 0x35,
    ERR_ASM_INVALID_OPERAND = 0x36,
    ERR_ASM_UNEXPECTED_DIRECTIVE = 0x37,
    ERR_ASM_UNEXPECTED_OPERAND = 0x38,
    ERR_ASM_EXPECTED_INSTRUCTION = 0x39,
    ERR_ASM_EXPECTED_OPERAND = 0x3a,
    ERR_ASM_EXPECTED_ORIG = 0x3b,
    ERR_ASM_EXPECTED_END = 0x3c,
    ERR_ASM_EXPECTED_COMMA = 0x3d,
    ERR_ASM_DUPLICATE_LABEL = 0x3e,
    ERR_ASM_UNDEFINED_LABEL = 0x3f,
    // Malformed instructions
    ERR_MALFORMED_INSTR = 0x50,     // Invalid/unsupported/reserved instruction
    ERR_MALFORMED_PADDING = 0x51,   // Expected correct padding in instruction
    ERR_MALFORMED_TRAP = 0x52,      // Invalid/unsupported trap vector
    ERR_UNAUTHORIZED_INSTR = 0x53,  // RTI (not in supervisor mode)
    // Runtime error
    ERR_ADDRESS_TOO_LOW = 0x61,   // Trying to load from memory before origin
    ERR_ADDRESS_TOO_HIGH = 0x62,  // Trying to load from memory after file end
    // 'Meta'
    ERR_UNIMPLEMENTED = 0xf1,
    ERR_UNREACHABLE = 0xf2,
} Error;

#endif
