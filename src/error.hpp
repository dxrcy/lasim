#ifndef ERROR_HPP
#define ERROR_HPP

#define UNIMPLEMENTED()                               \
    {                                                 \
        fprintf(stderr, "Not yet implemented.\n");    \
        exit(static_cast<int>(Error::UNIMPLEMENTED)); \
    }
#define UNREACHABLE()                                          \
    {                                                          \
        fprintf(stderr, "Unreachable code reached. Uh oh!\n"); \
        exit(static_cast<int>(Error::UNREACHABLE));            \
    }

#define OK_OR_RETURN(error)     \
    {                           \
        if (error != Error::OK) \
            return;             \
    }

#define SET_ERROR(_error, _kind) \
    if ((_error) == Error::OK)   \
        (_error) = Error::_kind;

#define RETURN_IF_FAILED(_failed) \
    if (_failed)                  \
        return;

// Each variant correspnds to a process exit code
enum class Error {
    OK = 0x00,             // No error
    CLI = 0x10,            // Parsing CLI arguments
    FILE = 0x20,           // Opening/reading file
    ASSEMBLE = 0x30,       // Parsing/assembling .asm
    EXECUTE = 0x40,        // Executing .obj
    UNIMPLEMENTED = 0x80,  // Feature not implemented
    UNREACHABLE = 0xff,    // Unreachable code was reached
};

#endif
