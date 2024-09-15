#include "assemble.cpp"
#include "cli.cpp"
#include "error.hpp"
#include "execute.cpp"

void try_run(Options &options);

int main(const int argc, const char *const *const argv) {
    Options options;
    parse_options(options, argc, argv);

    // TODO(feat): Print out different message for each error

    try_run(options);
    switch (ERROR) {
        case ERR_OK:
            break;
        default:
            printf("ERROR: 0x%04x\n", ERROR);
            return ERROR;
    }

    return ERR_OK;
}

void try_run(Options &options) {
    Error2 error = Error2::OK;

    const char *assemble_filename = nullptr;  // Assemble if !nullptr
    const char *execute_filename = nullptr;   // Execute if !nullptr

    switch (options.mode) {
        case Mode::ASSEMBLE_ONLY:
            assemble_filename = options.in_filename;
            break;
        case Mode::EXECUTE_ONLY:
            execute_filename = options.in_filename;
            break;
        case Mode::ASSEMBLE_EXECUTE:
            assemble_filename = options.in_filename;
            execute_filename = options.out_filename;
            break;
    }

    // Sanity check
    if (assemble_filename == nullptr && execute_filename == nullptr)
        UNREACHABLE();

    if (assemble_filename != nullptr) {
        assemble(assemble_filename, options.out_filename, error);
        if (error != Error2::OK) {
            fprintf(stderr, "Assembly failed.\n");
            exit(static_cast<int>(error));
        }
        printf("Assembled successfully.\n");
        OK_OR_RETURN();
    }

    if (execute_filename == nullptr) {
        execute(execute_filename);
        OK_OR_RETURN();
    }
}
