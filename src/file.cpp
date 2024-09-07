#ifndef FILE_CPP
#define FILE_CPP

#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memset

#include "error.hpp"
#include "globals.cpp"
#include "types.hpp"

// Swap high and low bytes of a word
#define swap_endianess(word) (((word) << 8) | ((word) >> 8))

Error read_obj_file_to_memory(const char *const filename) {
    FILE *const file = fopen(filename, "rb");
    size_t words_read;

    if (file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        return ERR_FILE_OPEN;
    }

    Word origin;
    words_read = fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, file);

    if (ferror(file)) {
        fprintf(stderr, "Could not read file %s\n", filename);
        return ERR_FILE_READ;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", filename);
        return ERR_FILE_TOO_SHORT;
    }

    Word start = swap_endianess(origin);

    /* printf("origin: 0x%04x\n", start); */

    char *const memory_at_file = reinterpret_cast<char *>(memory + start);
    const size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    words_read = fread(memory_at_file, WORD_SIZE, max_file_bytes, file);

    if (ferror(file)) {
        fprintf(stderr, "Could not read file %s\n", filename);
        return ERR_FILE_READ;
    }
    if (words_read < 1) {
        fprintf(stderr, "File is too short %s\n", filename);
        return ERR_FILE_TOO_SHORT;
    }
    if (!feof(file)) {
        fprintf(stderr, "File is too long %s\n", filename);
        return ERR_FILE_TOO_LONG;
    }

    Word end = start + words_read;

    // Mark undefined bytes for debugging
    memset(memory, 0xdd, start * WORD_SIZE);  // Before file
    memset(memory + end, 0xee,
           (MEMORY_SIZE - end) * WORD_SIZE);  // After file

    // TODO: Make this better !!
    // ^ Read file word-by-word, and swap endianess in the same loop
    for (size_t i = start; i < end; ++i) {
        memory[i] = swap_endianess(memory[i]);
    }

    /* printf("words read: %ld\n", words_read); */

    memory_file_bounds.start = start;
    memory_file_bounds.end = end;

    fclose(file);

    return ERR_OK;
}

#endif
