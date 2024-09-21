#include <cstdio>  // fprintf, getchar

#include "slice.cpp"

// TODO(refactor): Create header file for execute.cpp or extract functions
void print_registers(void);

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

void take_command(const char *&line, StringSlice &command) {
    // Ignore leading spaces
    while (isspace(line[0]))
        ++line;

    command.pointer = line;
    for (char ch; (ch = line[0]) != '\0'; ++line) {
        if (isspace(ch))
            break;
    }
    command.length = line - command.pointer;
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

    if (slice_starts_with("h", command)) {
        dprintf(
            "    h   Print usage\n"
            "    r   Print registers\n"
            "    s   Step next instruction\n"
            "    c   Continue execution until HALT\n"
            "    ms  Set value at memory location\n"
            "    mg  Print value at memory address\n"
            "    q   Quit all execution\n"
            "");
    } else if (slice_starts_with("r", command)) {
        print_registers();
    } else if (slice_starts_with("s", command)) {
        return DebuggerAction::STEP;
    } else if (slice_starts_with("c", command)) {
        return DebuggerAction::CONTINUE;
    } else if (slice_starts_with("q", command)) {
        return DebuggerAction::QUIT;
    } else if (slice_starts_with("m", command)) {
        dprintf("(Unimplemented...)\n");
    } else {
        dprintf("Unknown command ");
        print_string_slice(stddbg, command);
        dprintf(". Use `h` to show usage.\n");
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
