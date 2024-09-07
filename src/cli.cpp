#ifndef CLI_CPP
#define CLI_CPP

#include <cstdio>   // fprintf, stderr
#include <cstdlib>  // exit
#include <cstring>  // strcpy

#include "error.hpp"

#define PROGRAM_NAME "lc3"

#define MAX_FILENAME 255  // NOT including '\0'

#define DEFAULT_OUT_EXTENSION "obj"
#define DEFAULT_OUT_EXTENSION_SIZE (sizeof(DEFAULT_OUT_EXTENSION))

enum Mode {
    MODE_ASSEMBLE_EXECUTE,  // (default)
    MODE_ASSEMBLE_ONLY,     // -a
    MODE_EXECUTE_ONLY,      // -x
};

struct Options {
    enum Mode mode;
    char in_file[MAX_FILENAME + 1];
    char out_file[MAX_FILENAME + 1];
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
    options.mode = MODE_ASSEMBLE_EXECUTE;

    bool in_file_set = false;
    bool out_file_set = false;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (arg[0] != '-') {
            if (!in_file_set) {
                in_file_set = true;
                strcpy_max_size(options.in_file, arg, MAX_FILENAME);
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
                    strcpy_max_size(options.out_file, next_arg, MAX_FILENAME);
                }; break;

                // Assemble
                case 'a': {
                    if (options.mode == MODE_ASSEMBLE_EXECUTE) {
                        options.mode = MODE_ASSEMBLE_ONLY;
                    } else {
                        fprintf(stderr, "Cannot specify `-a` with `-x`\n");
                        print_usage_hint();
                        exit(ERR_CLI);
                    }
                }; break;

                // Execute
                case 'x': {
                    if (options.mode == MODE_ASSEMBLE_EXECUTE) {
                        options.mode = MODE_EXECUTE_ONLY;
                    } else {
                        fprintf(stderr, "Cannot specify `-x` with `-a`\n");
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

    if (options.mode == MODE_EXECUTE_ONLY) {
        if (out_file_set) {
            fprintf(stderr, "Cannot specify output file with `-x`\n");
            print_usage_hint();
            exit(ERR_CLI);
        }
    } else {
        // Default output filename based on input filename
        if (!out_file_set) {
            copy_filename_with_extension(options.out_file, options.in_file);
        }
    }
}

void print_usage_hint() {
    fprintf(stderr, "Use `" PROGRAM_NAME " -h` to show usage\n");
}

void print_usage() {
    fprintf(stderr,
            "USAGE:\n"
            "    " PROGRAM_NAME "    " PROGRAM_NAME
            " -h"
            " [-ax] [INPUT] [-o OUTPUT]\n"
            "MODE:\n"
            "    (default)      Assemble and execute\n"
            "    -a             Assembly only\n"
            "    -x             Execute only\n"
            "ARGUMENTS:\n"
            "        [INPUT]    Input filename (.asm, or for .obj for -x)\n"
            "    -o [OUTPUT]    Output filename\n"
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
    for (; i < MAX_FILENAME; ++i) {
        char ch = src[i];
        if (ch == '\0') break;
        dest[i] = ch;
        if (ch == '.') last_period = i;
    }
    if (last_period == 0) last_period = i;
    if (last_period + DEFAULT_OUT_EXTENSION_SIZE >= MAX_FILENAME) {
        last_period = MAX_FILENAME - DEFAULT_OUT_EXTENSION_SIZE;
    }
    dest[last_period] = '.';
    strcpy(dest + last_period + 1, DEFAULT_OUT_EXTENSION);
    dest[last_period + DEFAULT_OUT_EXTENSION_SIZE] = '\0';
}

#endif
