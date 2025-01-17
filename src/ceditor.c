#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios original_termios;

void die(const char *s) {
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

int main() {
    enableRawMode();
    while(1) {
        char c = '\0';
        if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        if(iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d (%c)\r\n", c, c);
        }
        if(c == 'q') break;
    }
    return 0;
}