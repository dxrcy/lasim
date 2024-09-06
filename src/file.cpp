#ifndef FILE_CPP
#define FILE_CPP

#include <cstdio>   // FILE, fprintf, etc
#include <cstring>  // memset

#include "globals.cpp"
#include "types.hpp"

// Swap high and low bytes of a word
#define swap_endianess(word) (((word) << 8) | ((word) >> 8))

void read_file_to_memory(const char *const filename, Word &start, Word &end) {
    FILE *const file = fopen(filename, "rb");

    if (file == nullptr) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(ERR_FILE_READ);
    }

    // TODO: Handle failure
    Word origin;
    fread(reinterpret_cast<char *>(&origin), WORD_SIZE, 1, file);
    start = swap_endianess(origin);

    /* printf("origin: 0x%04x\n", start); */

    // TODO: Handle failure
    char *const memory_at_file = reinterpret_cast<char *>(memory + start);
    const size_t max_file_bytes = (MEMORY_SIZE - start) * WORD_SIZE;
    const size_t words_read =
        fread(memory_at_file, WORD_SIZE, max_file_bytes, file);

    end = start + words_read;

    // Mark undefined bytes for debugging
    memset(memory, 0xdd, start * WORD_SIZE);  // Before file
    memset(memory + end, 0xee,
           (MEMORY_SIZE - end) * WORD_SIZE);  // After file

    // TODO: Make this better !!
    for (size_t i = start; i < end; ++i) {
        memory[i] = swap_endianess(memory[i]);
    }

    /* printf("words read: %ld\n", words_read); */

    fclose(file);
}

#endif
