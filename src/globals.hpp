#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <cstdlib>  // exit

#include "error.hpp"
#include "types.hpp"

static Error ERROR;  // Global error value

static Word memory[MEMORY_SIZE];

static Registers registers;

// Start and end addresses of file in memory
static struct {
    Word start;
    Word end;
} memory_file_bounds;

static bool stdout_on_new_line = true;  // Count start of stream as new line

#endif
