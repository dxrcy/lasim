#include "assemble.cpp"
#include "cli.cpp"
#include "error.hpp"
#include "execute.cpp"

int main(const int argc, const char *const *const argv) {
    Options options;
    parse_options(options, argc, argv);

    switch (options.mode) {
        case MODE_ASSEMBLE_ONLY:
            RETURN_IF_ERR(assemble(options.in_file, options.out_file));
            break;

        case MODE_EXECUTE_ONLY:
            RETURN_IF_ERR(execute(options.in_file));
            break;

        case MODE_ASSEMBLE_EXECUTE:
            RETURN_IF_ERR(assemble(options.in_file, options.out_file));
            RETURN_IF_ERR(execute(options.out_file));
            break;

        default:
            unreachable();
    }

    return ERR_OK;
}
