#include "assemble.cpp"
#include "cli.cpp"
#include "error.hpp"
#include "execute.cpp"

Error try_run(Options &options);

int main(const int argc, const char *const *const argv) {
    Options options;
    parse_options(options, argc, argv);

    const Error error = try_run(options);
    switch (error) {
        case ERR_OK:
            break;
        default:
            printf("ERROR: 0x%04x\n", error);
            return error;
    }

    return ERR_OK;
}

Error try_run(Options &options) {
    switch (options.mode) {
        case Mode::ASSEMBLE_ONLY:
            RETURN_IF_ERR(assemble(options.in_file, options.out_file));
            break;

        case Mode::EXECUTE_ONLY:
            RETURN_IF_ERR(execute(options.in_file));
            break;

        case Mode::ASSEMBLE_EXECUTE:
            RETURN_IF_ERR(assemble(options.in_file, options.out_file));
            RETURN_IF_ERR(execute(options.out_file));
            break;
    }
    return ERR_OK;
}
