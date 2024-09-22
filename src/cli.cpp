#ifndef CLI_CPP
#define CLI_CPP

#include <cstdio>   // fprintf, stderr
#include <cstdlib>  // exit
#include <cstring>  // strcpy

#include "error.hpp"

#define PROGRAM_NAME "lasim"

// A standard `FILENAME_MAX` of 4kiB is a bit large.
// Perhaps it's for the best if this program does not allow such large filenames
#undef FILENAME_MAX
#define FILENAME_MAX 256  // Includes '\0'

#define DEFAULT_OUT_EXTENSION "obj"
#define DEFAULT_OUT_EXTENSION_SIZE (sizeof(DEFAULT_OUT_EXTENSION))

enum class Mode {
    ASSEMBLE_EXECUTE,  // (default)
    ASSEMBLE_ONLY,     // -a
    EXECUTE_ONLY,      // -x
};

struct Options {
    Mode mode;
    // Empty string (file[0]=='\0') refers to stdin/stdout respectively
    char in_filename[FILENAME_MAX];
    char out_filename[FILENAME_MAX];
    bool debugger;
};

void parse_options(
    Options &options, const int argc, const char *const *const argv
);
void print_usage_hint(void);
void print_usage(void);
void strcpy_max_size(
    char *const dest, const char *const src, const size_t max_size
);
void copy_filename_with_extension(char *const dest, const char *const src);

void parse_options(
    Options &options, const int argc, const char *const *const argv
) {
    options.mode = Mode::ASSEMBLE_EXECUTE;
    options.debugger = false;

    bool in_file_set = false;
    bool out_file_set = false;

    // TODO(feat/ax): Write output file iff `-o` specified

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        // TODO(feat): Parse arguments raw after `--`

        if (arg[0] == '\0') {
            fprintf(stderr, "Argument cannot be empty\n");
            print_usage_hint();
            exit(static_cast<int>(Error::CLI));
        }

        // `-` as input file for stdin
        if (arg[0] != '-' || arg[1] == '\0') {
            if (!in_file_set) {
                in_file_set = true;
                if (arg[0] == '-') {
                    options.in_filename[0] = '\0';
                } else {
                    strcpy_max_size(options.in_filename, arg, FILENAME_MAX - 1);
                }
            } else {
                fprintf(stderr, "Unexpected argument: `%s`\n", arg);
                print_usage_hint();
                exit(static_cast<int>(Error::CLI));
            }
            continue;
        }

        ++arg;  // Move past `-`
        if (arg[0] == '\0') {
            fprintf(stderr, "Expected option name after `-`\n");
            print_usage_hint();
            exit(static_cast<int>(Error::CLI));
        }

        for (char option; (option = arg[0]) != '\0'; ++arg) {
            switch (option) {
                // Help
                case 'h': {
                    print_usage();
                    exit(static_cast<int>(Error::OK));
                }; break;

                // Output file
                case 'o': {
                    if (out_file_set) {
                        fprintf(stderr, "Cannot specify `-o` more than once\n");
                        print_usage_hint();
                        exit(static_cast<int>(Error::CLI));
                    }
                    out_file_set = true;
                    if (i + 1 >= argc) {
                        fprintf(stderr, "Expected argument for `-o`\n");
                        print_usage_hint();
                        exit(static_cast<int>(Error::CLI));
                    }
                    const char *next_arg = argv[++i];
                    // `-` as output filename for stdout
                    if (next_arg[0] == '-') {
                        if (next_arg[1] != '\0') {
                            fprintf(stderr, "Expected argument for `-o`\n");
                            print_usage_hint();
                            exit(static_cast<int>(Error::CLI));
                        }
                        options.out_filename[0] = '\0';
                    } else {
                        strcpy_max_size(
                            options.out_filename, next_arg, FILENAME_MAX - 1
                        );
                    }
                }; break;

                // Assemble
                case 'a':
                    switch (options.mode) {
                        case Mode::ASSEMBLE_EXECUTE:
                            options.mode = Mode::ASSEMBLE_ONLY;
                            break;
                        case Mode::ASSEMBLE_ONLY:
                            fprintf(
                                stderr, "Cannot specify `-a` more than once\n"
                            );
                            print_usage_hint();
                            exit(static_cast<int>(Error::CLI));
                        default:
                            fprintf(
                                stderr,
                                "Cannot specify `-a` with `-x`. Omit both "
                                "options "
                                "for default (assemble+execute) mode.\n"
                            );
                            print_usage_hint();
                            exit(static_cast<int>(Error::CLI));
                    };
                    break;

                // Execute
                case 'x': {
                    switch (options.mode) {
                        case Mode::ASSEMBLE_EXECUTE:
                            options.mode = Mode::EXECUTE_ONLY;
                            break;
                        case Mode::EXECUTE_ONLY:
                            fprintf(
                                stderr, "Cannot specify `-x` more than once\n"
                            );
                            print_usage_hint();
                            exit(static_cast<int>(Error::CLI));
                        default:
                            fprintf(
                                stderr,
                                "Cannot specify `-x` with `-a`. Omit both "
                                "options "
                                "for default (assemble+execute) mode.\n"
                            );
                            print_usage_hint();
                            exit(static_cast<int>(Error::CLI));
                    };
                }; break;

                // Debugger
                case 'd': {
                    if (options.debugger) {
                        fprintf(stderr, "Cannot specify `-d` more than once\n");
                        print_usage_hint();
                        exit(static_cast<int>(Error::CLI));
                    }
                    options.debugger = true;
                }; break;

                default:
                    fprintf(stderr, "Invalid option: `-%c`\n", option);
                    print_usage_hint();
                    exit(static_cast<int>(Error::CLI));
                    break;
            }
        }
    }

    if (!in_file_set) {
        fprintf(stderr, "No input file specified\n");
        print_usage_hint();
        exit(static_cast<int>(Error::CLI));
    }

    if (options.debugger && options.mode == Mode::ASSEMBLE_ONLY) {
        fprintf(stderr, "Cannot use debugger in assemble-only mode\n");
        print_usage_hint();
        exit(static_cast<int>(Error::CLI));
    }

    if (options.mode == Mode::EXECUTE_ONLY) {
        if (out_file_set) {
            fprintf(stderr, "Cannot specify output file with `-x`\n");
            print_usage_hint();
            exit(static_cast<int>(Error::CLI));
        }
    } else if (!out_file_set) {
        // Mode is a|ax, but no output file was specified
        // Default output filename based on input filename
        copy_filename_with_extension(options.out_filename, options.in_filename);
    } else if (options.mode == Mode::ASSEMBLE_EXECUTE &&
               options.out_filename[0] == '\0') {
        // Mode is ax, but output file was set as stdout (using `-`)
        // An intermediate file is required
        // Cannot use `-o -` in ax|x mode (x mode checked above)
        fprintf(
            stderr,
            "Cannot write output to stdout in default "
            "(assemble+execute) mode\n"
        );
        print_usage_hint();
        exit(static_cast<int>(Error::CLI));
    }
}

void print_usage_hint() {
    fprintf(stderr, "Use `" PROGRAM_NAME " -h` to show usage\n");
}

void print_usage() {
    fprintf(
        stderr,
        "LASIM: LC-3 Assembler & Simulator\n"
        "\n"
        "USAGE:\n"
        "    " PROGRAM_NAME
        " -h [-ax] [INPUT] [-o OUTPUT]\n"
        "MODE:\n"
        "    (default)      Assemble + Execute\n"
        "    -a             Assembly only\n"
        "    -x             Execute only\n"
        "ARGUMENTS:\n"
        "        [INPUT]    Input filename (.asm, or .obj for -x)\n"
        "                   Use '-' to read input from stdin\n"
        "    -o [OUTPUT]    Output filename\n"
        "                   Use '-' to write output to stdout (with -a)\n"
        "    -d             Debug program execution\n"
        "OPTIONS:\n"
        "    -h             Print usage\n"
        ""
    );
}

// Like `strncpy`, but ALWAYS null-terminate destination string
// `max_length` does NOT include NUL byte
void strcpy_max_size(
    char *const dest, const char *const src, const size_t max_length
) {
    size_t i = 0;
    for (; i < max_length; ++i) {
        if (src[i] == '\0')
            break;
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void copy_filename_with_extension(char *const dest, const char *const src) {
    size_t last_period = 0;
    size_t i = 0;
    for (; i < FILENAME_MAX - 1; ++i) {
        char ch = src[i];
        if (ch == '\0')
            break;
        dest[i] = ch;
        if (ch == '.')
            last_period = i;
    }
    if (last_period == 0)
        last_period = i;
    if (last_period + DEFAULT_OUT_EXTENSION_SIZE > FILENAME_MAX) {
        last_period = FILENAME_MAX - 1 - DEFAULT_OUT_EXTENSION_SIZE;
    }
    dest[last_period] = '.';
    strcpy(dest + last_period + 1, DEFAULT_OUT_EXTENSION);
    dest[last_period + DEFAULT_OUT_EXTENSION_SIZE] = '\0';
}

#endif
