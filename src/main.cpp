#include "error.hpp"
#include "execute.cpp"
#include "file.cpp"

int main(const int argc, const char *const *const argv) {
    if (argc != 2 || argv[1][0] == '-') {
        fprintf(stderr, "USAGE: lc3sim [FILE]\n");
        exit(ERR_CLI_ARGUMENTS);
    }

    const char *filename = argv[1];

    RETURN_IF_ERR(read_file_to_memory(filename));
    RETURN_IF_ERR(execute());

    return 0;
}
