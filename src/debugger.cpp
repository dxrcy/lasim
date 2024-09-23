#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include <cstdio>  // fprintf, getchar

#include "globals.hpp"
#include "slice.cpp"
#include "token.cpp"
#include "tty.cpp"
#include "types.hpp"

// TODO(feat): Overhaul debugger commands:
// Not all have to be added
//    reg           show all registers
//    get r         get a register value
//    set r v       set a register value
//    load a        set memory at address/label
//    store a v     show memory at address/label
//    list          list memory/instrs at PC/adress/label (readable)
//    dump          dump memory/instrs at PC/adress/label (hex)
//    reload        re-read file, resetting registers and memory
//    trap t        simulate trap
//    halt          simulate HALT
//    step n        execute next instruction
//    next          execute next instruction or subroutine
//    continue      continue till next HALT or breakpoint
//    finish        execute to end of current subroutine
//    quit          quit debugger, continue execution
//    exit          quit debugger and executor
// TODO(feat): How would these debugger commands work?
//    break?     set/remove breakpoint

// TODO(feat): Keep track of labels for debugger
//     Maybe make label_definitions list global/static
//     A symbol table file seems more hassle than it's worth

// TODO(refactor): Create header file for execute.cpp or extract functions
void print_on_new_line(void);
static char *halfbyte_string(const Word word);

#define MAX_DEBUGGER_COMMAND 20  // Includes '\0'
#define MAX_DEBUGGER_HISTORY 4

#define stddbg stderr

// TODO(refactor): Rename, extract other color codes
#define DEBUGGER_COLOR "\x1b[36m"

// Debugger message
// TODO(feat): Disable color with cli option
#define dprintf(...)                      \
    {                                     \
        if (!debugger_quiet) {            \
            fprintf(stddbg, __VA_ARGS__); \
            fflush(stddbg);               \
        }                                 \
    }
#define dprintfc(...)                        \
    {                                        \
        if (!debugger_quiet) {               \
            fprintf(stddbg, DEBUGGER_COLOR); \
            fprintf(stddbg, __VA_ARGS__);    \
            fprintf(stddbg, "\x1b[0m");      \
            fflush(stddbg);                  \
        }                                    \
    }
#define dprintfc_always(...)          \
    {                                 \
        fprintf(stddbg, __VA_ARGS__); \
        fflush(stddbg);               \
    }

// TODO(refactor): Move to state object, when that's implemented
// TODO(refactor): Maybe make all debugger state in a separate static object
static bool debugger_quiet = false;

// Only for debugger commands which affect program control-flow
enum class DebuggerAction {
    NONE,      // No control-flow action taken
    STEP,      // Execute next instruction
    CONTINUE,  // Continue until breakpoint or HALT
    QUIT,      // Quit debugger and simulator
    STOP,      // Stop debugger, continue simulator
};

// TODO(refactor/opt): Use string type with length
// TODO(rename): `Command` maybe `RawCommand` ?
typedef char Command[MAX_DEBUGGER_COMMAND];

// TODO(opt): Use ring buffer
typedef struct CommandHistory {
    Command list[MAX_DEBUGGER_HISTORY];
    size_t length = 0;
    size_t cursor = 0;
} CommandHistory;

static CommandHistory history;

enum class DebuggerCommand {
    UNKNOWN,
    REGISTERS,
    STEP,
    CONTINUE,
    MEMORY_GET,
    MEMORY_SET,
    QUIT,
    STOP,
};

// TODO(refactor): Create function prototypes
// TODO(lint): Use `void` param for prototypes of these functions
// TODO(refactor): Rename functions
// TODO(refactor): Use namespace ?

void print_registers(FILE *const file);
char condition_char(ConditionCode condition);

void push_history(const char *const buffer) {
    if (history.length >= MAX_DEBUGGER_HISTORY) {
        for (size_t i = 0; i < history.length - 1; ++i) {
            strcpy(history.list[i], history.list[i + 1]);
        }
    } else {
        ++history.length;
    }
    strcpy(history.list[history.length - 1], buffer);
    history.cursor = history.length;
}

void print_command_prompt() {
    dprintf("\r\x1b[K");
    dprintf("\x1b[1m");
    dprintfc("Command: ");
}

bool read_line(char *const buffer) {
    size_t length = 0;
    // TODO(feat): Add line cursor

    tty_nobuffer_noecho();
    while (true) {
        /* printf("buffer:  %lu\n", length); */
        /* printf("history: %lu\n", history.length); */
        /* printf("cursor:  %lu\n", history.cursor); */
        print_command_prompt();
        for (size_t i = 0; i < length; ++i) {
            dprintf("%c", buffer[i]);
        }

        int ch = getchar();

        if (ch == EOF) {
            if (length > 0) {
                // Defer `STOP` action for next command read
                // Treat as if input ended in a newline
                break;
            } else {
                tty_restore();
                dprintf("\n");
                return false;
            }
        }

        if (ch == '\n' || ch == ',')
            break;
        if (ch == '\r')
            continue;

        // Ignore leading whitespace
        // Useful when stdin is piped: don't echo leading whitespace
        if (isspace(ch) && length == 0)
            continue;

        if (ch == '\x7f' || ch == '\x08') {
            if (length > 0)
                --length;
        } else if (ch == '\x1b') {
            ch = getchar();
            if (ch != '[')
                continue;

            ch = getchar();
            switch (ch) {
                case 'A':
                    if (history.cursor > 0) {
                        --history.cursor;
                        strcpy(buffer, history.list[history.cursor]);
                        length = strlen(buffer);
                    }
                    break;
                case 'B':
                    if (history.cursor < history.length) {
                        ++history.cursor;
                        if (history.cursor == history.length) {
                            length = 0;
                        } else {
                            strcpy(buffer, history.list[history.cursor]);
                            length = strlen(buffer);
                        }
                    }
                    break;
                default:
                    break;
            }
        } else {
            if (length < MAX_DEBUGGER_COMMAND - 1) {
                buffer[length] = ch;
                ++length;
            }
        }
    }
    tty_restore();
    dprintf("\n");

    buffer[length] = '\0';
    if (length > 0) {
        push_history(buffer);
    }
    return true;
}

void take_whitespace(const char *&line) {
    // Ignore leading spaces
    while (isspace(line[0]))
        ++line;
}

DebuggerCommand take_command(const char *&line) {
    StringSlice command;
    take_whitespace(line);
    command.pointer = line;
    for (char ch; (ch = line[0]) != '\0'; ++line) {
        if (isspace(ch))
            break;
    }
    command.length = line - command.pointer;

    // These comparisons are CASE-INSENSITIVE!
    if (string_equals_slice("r", command) ||
        string_equals_slice("reg", command) ||
        string_equals_slice("registers", command)) {
        return DebuggerCommand::REGISTERS;
    }
    if (string_equals_slice("s", command) ||
        string_equals_slice("step", command)) {
        return DebuggerCommand::STEP;
    }
    if (string_equals_slice("c", command) ||
        string_equals_slice("cont", command) ||
        string_equals_slice("continue", command)) {
        return DebuggerCommand::CONTINUE;
    }
    if (string_equals_slice("mg", command) ||
        string_equals_slice("memg", command) ||
        string_equals_slice("mget", command) ||
        string_equals_slice("memget", command) ||
        string_equals_slice("memoryget", command)) {
        return DebuggerCommand::MEMORY_GET;
    }
    if (string_equals_slice("ms", command) ||
        string_equals_slice("mems", command) ||
        string_equals_slice("mset", command) ||
        string_equals_slice("memset", command) ||
        string_equals_slice("memoryset", command)) {
        return DebuggerCommand::MEMORY_SET;
    }
    if (string_equals_slice("q", command) ||
        string_equals_slice("quit", command)) {
        return DebuggerCommand::QUIT;
    }
    if (string_equals_slice("stop", command)) {
        return DebuggerCommand::STOP;
    }
    return DebuggerCommand::UNKNOWN;
}

bool expect_address(const char *&line, Word &addr) {
    take_whitespace(line);
    InitialSignWord integer;
    if (take_integer(line, integer) != 1 || integer.is_signed) {
        dprintfc("Expected address argument\n");
        return false;
    }
    addr = integer.value;
    // Reflects `memory_checked`
    if (addr < memory_file_bounds.start || addr > MEMORY_USER_MAX) {
        dprintfc("Memory address is out of bounds\n");
        return false;
    }
    return true;
}

bool expect_integer(const char *&line, Word &value) {
    take_whitespace(line);
    InitialSignWord integer;
    if (take_integer(line, integer) != 1) {
        dprintfc("Expected integer argument\n");
        return false;
    }
    value = integer.value;
    // TODO(fix): Check range correctly
    return true;
}

void print_integer_value(Word value) {
    // TODO(refactor): Combine functionality with `print_registers`
    // TODO(feat): Show ascii repr. if applicable
    // TODO(feat): Show instruction name/opcode repr. if applicable
    if (debugger_quiet) {
        dprintfc_always("0x%04hx\n", value);
    } else {
        dprintfc("       HEX    UINT    INT\n");
        dprintfc("    0x%04hx  %6hd  %5hu\n", value, value, value);
    }
}

DebuggerAction ask_debugger_command() {
    const char *line = nullptr;

    while (true) {
        Command line_buf;
        line = line_buf;
        // On EOF, continue without debugger
        if (!read_line(line_buf))
            return DebuggerAction::STOP;
        if (line_buf[0] != '\0')
            break;
    }

    DebuggerCommand command = take_command(line);

    // TODO(feat): Check for trailing operands

    switch (command) {
        case DebuggerCommand::REGISTERS: {
            if (!debugger_quiet) {
                dprintf(DEBUGGER_COLOR);
                print_registers(stddbg);
            }
        }; break;
        case DebuggerCommand::MEMORY_GET: {
            Word addr;
            if (!expect_address(line, addr))
                return DebuggerAction::NONE;
            Word value = memory[addr];
            dprintfc("Value at address 0x%04hx:\n", addr);
            print_integer_value(value);
        }; break;
        case DebuggerCommand::MEMORY_SET: {
            Word addr, value;
            if (!expect_address(line, addr))
                return DebuggerAction::NONE;
            if (!expect_integer(line, value))
                return DebuggerAction::NONE;
            memory[addr] = value;
            dprintfc("Modified value at address 0x%04hx\n", addr);
        }; break;
        case DebuggerCommand::STEP:
            return DebuggerAction::STEP;
            break;
        case DebuggerCommand::CONTINUE:
            return DebuggerAction::CONTINUE;
            break;
        case DebuggerCommand::QUIT:
            return DebuggerAction::QUIT;
            break;
        case DebuggerCommand::STOP:
            return DebuggerAction::STOP;
        default:
            dprintfc(
                "    h      Print usage\n"
                "    r      Print registers\n"
                "    s      Execute next instruction\n"
                "    c      Continue execution until breakpoint or HALT\n"
                "    mg     Print value at memory address\n"
                "    ms     Set value at memory location\n"
                /* "    rg     Print value of a register\n" */
                /* "    rs     Set value of a register\n" */
                "    q      Quit all execution\n"
                "    stop   Stop debugger, continue execution\n"
                ""
            );
    }

    return DebuggerAction::NONE;
}

void run_all_debugger_commands(
    bool &do_halt, bool &do_prompt, bool &do_debugger
) {
    while (true) {
        switch (ask_debugger_command()) {
            case DebuggerAction::STEP:
                return;

            case DebuggerAction::CONTINUE:
                do_prompt = false;
                return;

            case DebuggerAction::QUIT:
                do_halt = true;
                return;

            case DebuggerAction::STOP:
                do_debugger = false;
                return;

            case DebuggerAction::NONE:
                break;
        }
    }
}

// TODO(fix): Maybe specify file to print to ? for debugger
void print_registers(FILE *const file) {
    const int width = 27;
    const char *const box_h = "─";
    const char *const box_v = "│";
    const char *const box_tl = "╭";
    const char *const box_tr = "╮";
    const char *const box_bl = "╰";
    const char *const box_br = "╯";

    print_on_new_line();

    fprintf(file, "  %s", box_tl);
    for (size_t i = 0; i < width; ++i)
        fprintf(file, "%s", box_h);
    fprintf(file, "%s\n", box_tr);

    fprintf(file, "  %s ", box_v);
    fprintf(
        file,
        "pc: 0x%04hx          cc: %c",
        registers.program_counter,
        condition_char(registers.condition)
    );
    fprintf(file, " %s\n", box_v);

    fprintf(file, "  %s ", box_v);
    fprintf(file, "       HEX    UINT    INT");
    fprintf(file, " %s\n", box_v);

    for (int reg = 0; reg < GP_REGISTER_COUNT; ++reg) {
        const Word value = registers.general_purpose[reg];
        fprintf(file, "  %s ", box_v);
        fprintf(file, "r%d  0x%04hx  %6hd  %5hu", reg, value, value, value);
        fprintf(file, " %s\n", box_v);
    }

    fprintf(file, "  %s", box_bl);
    for (size_t i = 0; i < width; ++i)
        fprintf(file, "%s", box_h);
    fprintf(file, "%s\n", box_br);

    stdout_on_new_line = true;
}

char condition_char(ConditionCode condition) {
    switch (condition) {
        case ConditionCode::NEGATIVE:
            return 'N';
        case ConditionCode::ZERO:
            return 'Z';
        case ConditionCode::POSITIVE:
            return 'P';
        default:
            return '?';
    }
}

#endif
