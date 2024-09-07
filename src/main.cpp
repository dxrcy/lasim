#include "execute.cpp"
#include "file.cpp"
#include "types.hpp"

int main(const int argc, const char *const *const argv) {
    if (argc != 2 || argv[1][0] == '-') {
        fprintf(stderr, "USAGE: lc3sim [FILE]\n");
        exit(ERR_CLI_ARGUMENTS);
    }

    const char *filename = argv[1];

    Word file_start, file_end;
    read_file_to_memory(filename, file_start, file_end);

    /* for (size_t i = file_start - 2; i < file_end + 2; ++i) { */
    /*     printf("FILE: 0x%04lx: 0x%04hx\n", i, memory[i]); */
    /* } */

    execute(file_start, file_end);

    printf("\n");

    return 0;
}
