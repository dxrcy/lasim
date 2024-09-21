#include <cstdio>  // fprintf, getchar

// TODO(refactor): Create header file for execute.cpp or extract functions
void print_registers(void);

#define MAX_DEBUG_COMMAND 20  // Includes '\0'

// Debugger message
#define dprintf(...)                  \
    {                                 \
        fprintf(stderr, "\x1b[36m");  \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\x1b[0m");   \
        fflush(stderr);               \
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

DebuggerAction ask_debugger_command() {
    const char *command = nullptr;

    while (true) {
        char command_buf[MAX_DEBUG_COMMAND];
        command = command_buf;
        fprintf(stderr, "\x1b[1m");
        dprintf("Command: ");
        if (!read_line(command_buf, MAX_DEBUG_COMMAND))
            return DebuggerAction::NONE;
        if (command_buf[0] != '\0')
            break;
    }

    switch (command[0]) {
        case 'h':
            dprintf(
                "    h   Print usage\n"
                "    r   Print registers\n"
                "    s   Step next instruction\n"
                "    c   Continue execution\n"
                "    ms  Set value at memory location\n"
                "    mg  Print value at memory address\n"
                "    q   Quit all execution\n"
                "");
            break;
        case 'r':
            print_registers();
            break;
        case 's':
            return DebuggerAction::STEP;
        case 'c':
            return DebuggerAction::CONTINUE;
        case 'q':
            return DebuggerAction::QUIT;
        case 'm':
            dprintf("(Unimplemented...)\n");
            break;
        default:
            dprintf(
                "Unknown command. Use `h` to show usage.\n"
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
