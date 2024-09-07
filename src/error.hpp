#ifndef ERROR_HPP
#define ERROR_HPP

// Maybe this can print some log in debug mode ?
#define IGNORE_ERR(result) result

#define RETURN_IF_ERR(result)                \
    {                                        \
        /* `input` may be a function call */ \
        const Error error = (result);        \
        if (error != ERR_OK) {               \
            return error;                    \
        }                                    \
    }

typedef enum Error {
    // Ok
    ERR_OK = 0x00,
    // CLI arguments
    ERR_CLI_ARGUMENTS = 0x10,
    // File operations
    ERR_FILE_OPEN = 0x20,
    ERR_FILE_READ = 0x21,
    ERR_FILE_TOO_SHORT = 0x22,  // No instructions
    ERR_FILE_TOO_LONG = 0x23,   // Too many instructions to fit in memory
    // Malformed instructions
    ERR_MALFORMED_INSTR = 0x30,     // Invalid/unsupported/reserved instruction
    ERR_MALFORMED_PADDING = 0x31,   // Expected correct padding in instruction
    ERR_MALFORMED_TRAP = 0x32,      // Invalid/unsupported trap vector
    ERR_UNAUTHORIZED_INSTR = 0x33,  // RTI (not in supervisor mode)
    // Runtime error
    ERR_ADDRESS_TOO_LOW = 0x40,   // Trying to load from memory before origin
    ERR_ADDRESS_TOO_HIGH = 0x40,  // Trying to load from memory after file end
    // Unimplemented
    ERR_UNIMPLEMENTED = 0x70,
} Error;

#endif
