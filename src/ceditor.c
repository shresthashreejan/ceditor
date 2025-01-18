// Includes
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// Defines
#define CTRL_KEY(k) ((k) & 0x1f)

// Data
struct termios original_termios;

// Terminal
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1) die("tcsetattr");
}

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &original_termios)) die("tcgetattr");
    /*
        - Disabling raw mode at program exit
        - atexit() comes from stdlib.h
    */
    atexit(disableRawMode);

    struct termios raw = original_termios;
    /*
        - - ICRNL: CR stands for carriage returns and NL stands for new line
    */
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    /*
        - ICANON flag allows turning canonical mode off, which means reading input byte by byte instead of line by line
        - ISIG flag disabled to disable SIGINT (terminate signal) and SIGTSTP (suspend signal)
    */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    /*
        - VMIN value sets the min number of bytes input requires before read() returns
        - VTIME value sets the max time to wait before read() returns
    */
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

// Output

void editorDrawRows() {
    int y;
    for (y=0;y<24;y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {
    /*
        - Escape sequences always start with an escape character (27 or \x1b) followed by a [ character
    */
    write(STDOUT_FILENO, "\x1b[2J", 4);
    /*
        - Escape sequence to reposition cursor at the top left corner
    */
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

// Input
void editorProcessKeypress() {
    char c = editorReadKey();
    switch(c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

// Init
int main() {
    enableRawMode();
    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}