#ifndef CLI_CPP
#define CLI_CPP

#include <cstdio>   // fprintf, stderr
#include <cstdlib>  // exit
#include <cstring>  // strcpy

#include "error.hpp"

#define PROGRAM_NAME "lasim"

// A standard `FILENAME_MAX` of 4kB is a bit large.
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
    char in_file[FILENAME_MAX];
    char out_file[FILENAME_MAX];
};

void parse_options(Options &options, const int argc,
                   const char *const *const argv);
void print_usage_hint(void);
void print_usage(void);
void strcpy_max_size(char *const dest, const char *const src,
                     const size_t max_size);
void copy_filename_with_extension(char *const dest, const char *const src);

void parse_options(Options &options, const int argc,
                   const char *const *const argv) {
    options.mode = Mode::ASSEMBLE_EXECUTE;

    bool in_file_set = false;
    bool out_file_set = false;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (arg[0] == '\0') {
            fprintf(stderr, "Argument cannot be empty\n");
            print_usage_hint();
            exit(ERR_CLI);
        }

        // `-` as input file for stdin
        if (arg[0] != '-' || arg[1] == '\0') {
            if (!in_file_set) {
                in_file_set = true;
                if (arg[0] == '-') {
                    options.in_file[0] = '\0';
                } else {
                    strcpy_max_size(options.in_file, arg, FILENAME_MAX - 1);
                }
            } else {
                fprintf(stderr, "Unexpected argument: `%s`\n", arg);
                print_usage_hint();
                exit(ERR_CLI);
            }
            continue;
        }

        ++arg;  // Move past `-`
        if (arg[0] == '\0') {
            fprintf(stderr, "Expected option name after `-`\n");
            print_usage_hint();
            exit(ERR_CLI);
        }

        for (char option; (option = arg[0]) != '\0'; ++arg) {
            switch (option) {
                // Help
                case 'h': {
                    print_usage();
                    exit(ERR_OK);
                }; break;

                // Output file
                case 'o': {
                    out_file_set = true;
                    if (i + 1 >= argc) {
                        fprintf(stderr, "Expected argument for `-o`\n");
                        print_usage_hint();
                        exit(ERR_CLI);
                    }
                    const char *next_arg = argv[++i];
                    // `-` as output filename for stdout
                    if (next_arg[0] == '-') {
                        if (next_arg[1] != '\0') {
                            fprintf(stderr, "Expected argument for `-o`\n");
                            print_usage_hint();
                            exit(ERR_CLI);
                        }
                        options.out_file[0] = '\0';
                    } else {
                        strcpy_max_size(options.out_file, next_arg,
                                        FILENAME_MAX - 1);
                    }
                }; break;

                // Assemble
                case 'a': {
                    if (options.mode == Mode::ASSEMBLE_EXECUTE) {
                        options.mode = Mode::ASSEMBLE_ONLY;
                    } else {
                        fprintf(
                            stderr,
                            "Cannot specify `-a` with `-x`. Omit both options "
                            "for default (assemble+execute) mode.\n");
                        print_usage_hint();
                        exit(ERR_CLI);
                    }
                }; break;

                // Execute
                case 'x': {
                    if (options.mode == Mode::ASSEMBLE_EXECUTE) {
                        options.mode = Mode::EXECUTE_ONLY;
                    } else {
                        fprintf(
                            stderr,
                            "Cannot specify `-x` with `-a`. Omit both options "
                            "for default (assemble+execute) mode.\n");
                        print_usage_hint();
                        exit(ERR_CLI);
                    }
                }; break;

                default:
                    fprintf(stderr, "Invalid option: `-%c`\n", option);
                    print_usage_hint();
                    exit(ERR_CLI);
                    break;
            }
        }
    }

    if (!in_file_set) {
        fprintf(stderr, "No input file specified\n");
        print_usage_hint();
        exit(ERR_CLI);
    }

    // TODO(refactor): Make this `if` chain nicer

    if (options.mode == Mode::EXECUTE_ONLY) {
        if (out_file_set) {
            fprintf(stderr, "Cannot specify output file with `-x`\n");
            print_usage_hint();
            exit(ERR_CLI);
        }
    } else {
        // Default output filename based on input filename
        if (!out_file_set) {
            copy_filename_with_extension(options.out_file, options.in_file);
        } else {
            // An intermediate file is required
            // Cannot use `-o -` without `-a`
            // `-x` case is checked above
            if (options.mode == Mode::ASSEMBLE_EXECUTE &&
                options.out_file[0] == '\0') {
                fprintf(stderr,
                        "Cannot write output to stdout in default "
                        "(assemble+execute) mode\n");
                print_usage_hint();
                exit(ERR_CLI);
            }
        }
    }
}

void print_usage_hint() {
    fprintf(stderr, "Use `" PROGRAM_NAME " -h` to show usage\n");
}

void print_usage() {
    fprintf(stderr,
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
            "OPTIONS:\n"
            "    -h             Print usage\n"
            "");
}

// Better version of `strncpy`
void strcpy_max_size(char *const dest, const char *const src,
                     const size_t max_size) {
    size_t i = 0;
    for (; i < max_size; ++i) {
        if (src[i] == '\0') break;
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

void copy_filename_with_extension(char *const dest, const char *const src) {
    size_t last_period = 0;
    size_t i = 0;
    for (; i < FILENAME_MAX - 1; ++i) {
        char ch = src[i];
        if (ch == '\0') break;
        dest[i] = ch;
        if (ch == '.') last_period = i;
    }
    if (last_period == 0) last_period = i;
    if (last_period + DEFAULT_OUT_EXTENSION_SIZE > FILENAME_MAX) {
        last_period = FILENAME_MAX - 1 - DEFAULT_OUT_EXTENSION_SIZE;
    }
    dest[last_period] = '.';
    strcpy(dest + last_period + 1, DEFAULT_OUT_EXTENSION);
    dest[last_period + DEFAULT_OUT_EXTENSION_SIZE] = '\0';
}

#endif
