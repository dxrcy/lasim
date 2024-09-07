#ifndef GLOBALS_CPP
#define GLOBALS_CPP

#include <cstdlib>  // exit

#include "types.hpp"

#define exit(code)     \
    {                  \
        free_memory(); \
        exit(code);    \
    }

// Lifetime:
//  - Exists for program lifetime, but must still be deleted before exit
//  - Use `exit` macro to automatically free before exiting
//  - Dynamically allocated due to large size
// !! Do not access memory without checking first with `memory_check(addr)`
static Word *memory = new Word[MEMORY_SIZE];

static Registers registers;

// Start and end addresses of file in memory
static struct {
    Word start;
    Word end;
} memory_file_bounds;

// Does not need to be called when using `exit` macro
void free_memory() {
    delete[] memory;
    memory = nullptr;
}

static bool stdout_on_new_line = true;  // Count start of stream as new line

#endif
