#ifndef SLICE_CPP
#define SLICE_CPP

#include <cctype>
#include <cstddef>
#include <cstdio>

// Temporary reference to a substring of a line
// Must be copied if used after line is overwritten
typedef struct StringSlice {
    const char *pointer;
    size_t length;
} StringSlice;

bool string_equals_slice(const char *const target, const StringSlice candidate);
void copy_string_slice_to_string(char *dest, const StringSlice src);
void print_string_slice(FILE *const &file, const StringSlice &slice);

// TODO(refactor): Rename to `slice_equals[_string]`
bool string_equals_slice(const char *const target,
                         const StringSlice candidate) {
    size_t i = 0;
    for (; i < candidate.length; ++i) {
        // Will return false if any character mismatches
        //     OR target string is shorter (mismatch of NUL with candidate char)
        // Therefore no chance of reading past allocated length
        if (tolower(candidate.pointer[i]) != tolower(target[i]))
            return false;
    }
    // Target string is longer than candidate
    if (target[i] != '\0')
        return false;
    return true;
}

// TODO(lint): this is unused so can be removed
bool slice_starts_with(const char *const prefix, const StringSlice candidate) {
    size_t i = 0;
    for (; i < candidate.length; ++i) {
        // Prefix has ended. Must have matched
        if (prefix[i] == '\0')
            return true;
        // Will return false if any character mismatches
        if (tolower(candidate.pointer[i]) != tolower(prefix[i]))
            return false;
    }
    // Target string is longer than candidate
    if (prefix[i] != '\0')
        return false;
    return true;
}

// Assumes `dest` can hold src.length+1 characters
void copy_string_slice_to_string(char *dest, const StringSlice src) {
    for (size_t i = 0; i < src.length; ++i) {
        dest[i] = src.pointer[i];
    }
    dest[src.length] = '\0';
}

void print_string_slice(FILE *const &file, const StringSlice &slice) {
    for (size_t i = 0; i < slice.length; ++i) {
        fprintf(file, "%c", slice.pointer[i]);
    }
}

#endif
