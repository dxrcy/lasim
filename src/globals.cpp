#ifndef GLOBALS_CPP
#define GLOBALS_CPP

#include <cstdlib>  // exit

#include "types.hpp"

#define exit(code)     \
    {                  \
        free_memory(); \
        exit(code);    \
    }

// Exists for program lifetime, but must still be deleted before exit
// Use `EXIT` macro to automatically free before exiting
// Dynamically allocated due to large size
static Word *memory = new Word[MEMORY_SIZE];
static Registers registers;

// Does not need to be called when using `EXIT` macro
void free_memory() {
    delete[] memory;
    memory = nullptr;
}

#endif
