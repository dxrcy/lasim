#ifndef GLOBALS_CPP
#define GLOBALS_CPP

#include <cstdlib>  // exit

#include "types.hpp"

static Word memory[MEMORY_SIZE];

static Registers registers;

// Start and end addresses of file in memory
static struct {
    Word start;
    Word end;
} memory_file_bounds;

static bool stdout_on_new_line = true;  // Count start of stream as new line

#endif
