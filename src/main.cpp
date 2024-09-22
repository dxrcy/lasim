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
    ObjectFile object;

    if (options.debugger_quiet) {
        debugger_quiet = true;
    }

    switch (options.mode) {
        case Mode::ASSEMBLE_ONLY: {
            object.kind = ObjectFile::FILE;
            object.filename = options.out_filename;
            assemble(options.in_filename, object, error);
            if (error != Error::OK)
                return error;
        }; break;

        case Mode::EXECUTE_ONLY: {
            object.kind = ObjectFile::FILE;
            object.filename = options.in_filename;
            execute(object, options.debugger, error);
            if (error != Error::OK)
                return error;
        }; break;

        case Mode::ASSEMBLE_EXECUTE: {
            object.kind = ObjectFile::MEMORY;
            assemble(options.in_filename, object, error);
            if (error != Error::OK)
                return error;
            execute(object, options.debugger, error);
            if (error != Error::OK)
                return error;
        }; break;
    }

    return Error::OK;
}
