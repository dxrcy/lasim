#include "assemble.cpp"
#include "cli.cpp"
#include "error.hpp"
#include "execute.cpp"

Error try_run(Options &options);

int main(const int argc, const char *const *const argv) {
    Options options;
    parse_options(options, argc, argv);  // Exits on error

    Error error = try_run(options);

    switch (error) {
        case Error::OK:
            break;
        case Error::ASSEMBLE:
            fprintf(stderr, "Failed to assemble.\n");
            break;
        default:
            break;
    }

    return static_cast<int>(error);
}

Error try_run(Options &options) {
    Error error = Error::OK;

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
        if (error != Error::OK)
            return error;
    }

    if (execute_filename != nullptr) {
        execute(execute_filename, error);
        if (error != Error::OK)
            return error;
    }

    return Error::OK;
}
