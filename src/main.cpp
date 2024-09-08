#include "cli.cpp"
#include "error.hpp"
#include "execute.cpp"

int main(const int argc, const char *const *const argv) {
    Options options;
    parse_options(options, argc, argv);

    if (options.mode != MODE_EXECUTE_ONLY) {
        fprintf(stderr,
                "Only execute-only mode (-x) is currently supported.\n");
        return ERR_UNIMPLEMENTED;
    }

    RETURN_IF_ERR(execute(options.in_file));

    return ERR_OK;
}
