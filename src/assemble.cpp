#ifndef ASSEMBLE_CPP
#define ASSEMBLE_CPP

#include <cstdio>  // FILE, fprintf, etc

#include "error.hpp"

Error assemble(const char *const asm_file, const char *const obj_file);

Error assemble(const char *const asm_file, const char *const obj_file) {
    fprintf(stderr, "Assembler not yet implemented\n");
    return ERR_UNIMPLEMENTED;
}

#endif
