#ifndef TTY_CPP
#define TTY_CPP

#include <termios.h>  // termios, etc
#include <unistd.h>   // STDIN_FILENO

static struct termios tty;

void tty_get() { tcgetattr(STDIN_FILENO, &tty); }

void tty_apply() { tcsetattr(STDIN_FILENO, TCSANOW, &tty); }

void tty_nobuffer_noecho() {
    tty_get();
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty_apply();
}

void tty_nobuffer_yesecho() {
    tty_get();
    tty.c_lflag &= ~ICANON;
    tty.c_lflag |= ECHO;
    tty_apply();
}

void tty_restore() {
    tty.c_lflag |= ICANON;
    tty.c_lflag |= ECHO;
    tty_apply();
}

#endif
