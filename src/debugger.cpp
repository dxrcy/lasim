#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include <cstdio>  // fprintf, getchar

#include "globals.hpp"
#include "slice.cpp"
#include "token.cpp"
#include "types.hpp"

// TODO(refactor): Create header file for execute.cpp or extract functions
void print_on_new_line(void);
static char *halfbyte_string(const Word word);

void print_registers(void);
char condition_char(ConditionCode condition);

#define MAX_DEBUG_COMMAND 20  // Includes '\0'

#define stddbg stderr

// Debugger message
#define dprintf(...)                  \
    {                                 \
        fprintf(stddbg, "\x1b[36m");  \
        fprintf(stddbg, __VA_ARGS__); \
        fprintf(stddbg, "\x1b[0m");   \
        fflush(stddbg);               \
    }

// Only for debugger commands which affect program control flow
enum class DebuggerAction {
    NONE,
    STEP,
    CONTINUE,
    QUIT,
};

// TODO(refactor): Create function prototypes
// TODO(lint): Use `void` param for prototypes of these functions

bool read_line(char *buffer, size_t max_size) {
    int ch;
    // Read characters until EOL or EOF
    size_t len = 0;
    for (; len < max_size - 1; ++len) {
        ch = getchar();
        if (ch == EOF)
            return false;
        if (ch == '\n')
            break;
        if (ch == '\r')
            continue;
        buffer[len] = ch;
    }
    if (len >= max_size - 1) {
        // Flush trailing characters until end of line
        for (; (ch = getchar(), ch != '\n' && ch != EOF);)
            ;
    }
    buffer[len] = '\0';
    return true;
}

void take_whitespace(const char *&line) {
    // Ignore leading spaces
    while (isspace(line[0]))
        ++line;
}

void take_command(const char *&line, StringSlice &command) {
    take_whitespace(line);
    command.pointer = line;
    for (char ch; (ch = line[0]) != '\0'; ++line) {
        if (isspace(ch))
            break;
    }
    command.length = line - command.pointer;
}

bool expect_address(const char *&line, Word &addr) {
    take_whitespace(line);
    InitialSignWord integer;
    if (take_integer(line, integer) != 1 || integer.is_signed) {
        dprintf("Expected address argument\n");
        return false;
    }
    addr = integer.value;
    // Reflects `memory_checked`
    // TODO(fix): Messages should be different
    if (addr < memory_file_bounds.start) {
        dprintf("Cannot access non-user memory (before user memory)\n");
        return false;
    }
    if (addr > MEMORY_USER_MAX) {
        dprintf("Cannot access non-user memory (after user memory)\n");
        return false;
    }
    return true;
}

bool expect_integer(const char *&line, Word &value) {
    take_whitespace(line);
    InitialSignWord integer;
    if (take_integer(line, integer) != 1) {
        dprintf("Expected integer argument\n");
        return false;
    }
    value = integer.value;
    // TODO(fix): This won't check range correctly
    if (integer.is_signed)
        value = static_cast<Word>(-value);
    return true;
}

void print_integer_value(Word value) {
    // TODO(refactor): Combine functionality with `print_registers`
    // TODO(feat): Show ascii repr. if applicable
    // TODO(feat): Show instruction name/opcode repr. if applicable
    dprintf("       HEX    UINT    INT\n");
    dprintf("    0x%04hx  %6hd  %5hu\n", value, value, value);
}

DebuggerAction ask_debugger_command() {
    const char *line = nullptr;

    while (true) {
        char line_buf[MAX_DEBUG_COMMAND];
        line = line_buf;
        fprintf(stddbg, "\x1b[1m");
        dprintf("Command: ");
        if (!read_line(line_buf, MAX_DEBUG_COMMAND))
            return DebuggerAction::NONE;
        if (line_buf[0] != '\0')
            break;
    }

    StringSlice command;
    take_command(line, command);

    if (string_equals_slice("r", command)) {
        print_registers();
    } else if (string_equals_slice("s", command)) {
        return DebuggerAction::STEP;
    } else if (string_equals_slice("c", command)) {
        return DebuggerAction::CONTINUE;
    } else if (string_equals_slice("q", command)) {
        return DebuggerAction::QUIT;
    } else if (string_equals_slice("mg", command)) {
        Word addr;
        if (!expect_address(line, addr))
            return DebuggerAction::NONE;
        Word value = memory[addr];
        dprintf("Value at address 0x%04hx:\n", addr);
        print_integer_value(value);
    } else if (string_equals_slice("ms", command)) {
        Word addr, value;
        if (!expect_address(line, addr))
            return DebuggerAction::NONE;
        if (!expect_integer(line, value))
            return DebuggerAction::NONE;
        memory[addr] = value;
        dprintf("Modified value at address 0x%04hx\n", addr);
    } else {
        dprintf(
            "    h   Print usage\n"
            "    r   Print registers\n"
            "    s   Step next instruction\n"
            "    c   Continue execution until HALT\n"
            "    mg  Print value at memory address\n"
            "    ms  Set value at memory location\n"
            /* "    rg  Print value of a register\n" */
            /* "    rs  Set value of a register\n" */
            "    q   Quit all execution\n"
            "");
    }

    return DebuggerAction::NONE;
}

void run_all_debugger_commands(bool &do_halt, bool &do_prompt) {
    while (true) {
        switch (ask_debugger_command()) {
            case DebuggerAction::QUIT:
                do_halt = true;
                return;

            case DebuggerAction::STEP:
                return;

            case DebuggerAction::CONTINUE:
                do_prompt = false;
                return;

            case DebuggerAction::NONE:
                break;
        }
    }
}

// TODO(fix): Maybe specify file to print to ? for debugger
void print_registers() {
    const int width = 27;
    const char *const box_h = "─";
    const char *const box_v = "│";
    const char *const box_tl = "╭";
    const char *const box_tr = "╮";
    const char *const box_bl = "╰";
    const char *const box_br = "╯";

    print_on_new_line();

    printf("  %s", box_tl);
    for (size_t i = 0; i < width; ++i)
        printf("%s", box_h);
    printf("%s\n", box_tr);

    printf("  %s ", box_v);
    printf("pc: 0x%04hx          cc: %c", registers.program_counter,
           condition_char(registers.condition));
    printf(" %s\n", box_v);

    printf("  %s ", box_v);
    printf("       HEX    UINT    INT");
    printf(" %s\n", box_v);

    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        const Word value = registers.general_purpose[reg];
        printf("  %s ", box_v);
        printf("r%d  0x%04hx  %6hd  %5hu", reg, value, value, value);
        printf(" %s\n", box_v);
    }

    printf("  %s", box_bl);
    for (size_t i = 0; i < width; ++i)
        printf("%s", box_h);
    printf("%s\n", box_br);

    stdout_on_new_line = true;
}

char condition_char(ConditionCode condition) {
    switch (condition) {
        case 0b100:
            return 'N';
        case 0b010:
            return 'Z';
        case 0b001:
            return 'P';
        default:
            return '?';
    }
}

#endif
