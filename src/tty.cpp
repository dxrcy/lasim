#ifndef TTY_CPP
#define TTY_CPP

#include <termios.h>  // termios, etc
#include <unistd.h>   // STDIN_FILENO

static struct termios stdin_tty;

void tty_get() {
    tcgetattr(STDIN_FILENO, &stdin_tty);
}

void tty_nobuffer_noecho() {
    tty_get();
    stdin_tty.c_lflag &= ~ICANON;
    stdin_tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &stdin_tty);
}

void tty_restore() {
    stdin_tty.c_lflag |= ICANON;
    stdin_tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &stdin_tty);
}

#endif
