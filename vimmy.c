#include "vimmy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct winsize window_size;
struct termios orig_termios;

int cx, cy;
Mode mode = NORMAL_MODE;

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); 
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode); // restore terminal on exit

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/*
 * this function redraws the entire screen after a keypress. without this,
 * we wouldn't see the cursor move, and the text updates wouldn't appear on the
 * actual screen.
 */
void refreshScreen(Buffer *buf) {
    write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor
    write(STDOUT_FILENO, "\x1b[H", 3);    // move to top-left

    for (int i = 0; i < window_size.ws_row; i++) {
        if (i < buf->num_rows) {
            write(STDOUT_FILENO, buf->rows[i].chars, buf->rows[i].len);
        } else {
            write(STDOUT_FILENO, "~", 1);
        }
        write(STDOUT_FILENO, "\x1b[K", 3); // clear rest of line
        if (i < window_size.ws_row - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }

    char cursorBuf[32];
    int len =
        snprintf(cursorBuf, sizeof(cursorBuf), "\x1b[%d;%dH", cy + 1, cx + 1);
    write(STDOUT_FILENO, cursorBuf, len);
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
}

int getWindowSize() {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) != 0) {
        perror("ioctl failed!");
        return 1;
    }
    return 0;
}

/*
 * This func is written by ai. essentially reads a file and expands array if we
 * have too much data; like dynamic array
 */
void openFile(Buffer *buf, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        /* strip trailing newline */
        while (linelen > 0 &&
               (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;

        buf->rows = realloc(buf->rows, sizeof(Row) * (buf->num_rows + 1));
        Row *row = &buf->rows[buf->num_rows];
        row->len = linelen;
        row->chars = malloc(linelen + 1);
        memcpy(row->chars, line, linelen);
        row->chars[linelen] = '\0';
        buf->num_rows++;
    }

    free(line);
    fclose(fp);
}

void row_delete_char(Row *row, int cx) {
    if (cx <= 0 || cx > row->len)
        return;

    memmove(&row->chars[cx - 1], &row->chars[cx], row->len - cx + 1);
    row->len--;

    row->chars = realloc(row->chars, row->len + 1);
}

void buffer_delete_row(Buffer *buf, int cy) {
    if (cy < 0 || cy >= buf->num_rows)
        return;

    int num_rows_to_move = buf->num_rows - cy - 1;
    if (num_rows_to_move > 0) {
        memmove(&buf->rows[cy], &buf->rows[cy + 1], sizeof(Row) * num_rows_to_move);
    }

    buf->num_rows--;

    if (buf->num_rows == 0) {
        free(buf->rows);
        buf->rows = NULL;
    } else {
        buf->rows = realloc(buf->rows, sizeof(Row) * buf->num_rows);
    }
}

int main(int argc, char *argv[]) {
    enableRawMode();
    getWindowSize();

    Buffer buf = {NULL, 0};
    if (argc >= 2)
        openFile(&buf, argv[1]);

    while (1) {
        char c;
        refreshScreen(&buf);

        read(STDIN_FILENO, &c, 1);

        // normal mode
        if (mode == NORMAL_MODE) {
            switch (c) {
                case 'h': {
                    if (cx > 0)
                        cx--;
                    break;
                }
                case 'j': {
                    if (cy + 1 < window_size.ws_row)
                        cy++;
                    break;
                }
                case 'k': {
                    if (cy > 0)
                        cy--;
                    break;
                }
                case 'l': {
                    if (cx + 1 < window_size.ws_col)
                        cx++;
                    break;
                }
                case 'q': {
                    return 0;
                }
                case 'i': {
                    mode = INSERT_MODE;
                    break;
                }
                case 'a': {
                    if (cx + 1 < window_size.ws_col)
                        cx++;
                    mode = INSERT_MODE;
                    break;
                }
                case 'd': {
                    // wait for another input
                    while (1) {
                        char cc;
                        refreshScreen(&buf);
                        read(STDIN_FILENO, &cc, 1);
                        switch (cc) {
                            case 'd': {
                                buffer_delete_row(&buf, cy);
                                break;
                            }
                        }
                    }
                }
            }
            // insert mode
        } else {
            switch (c) {
                case '\x1b': {
                    mode = NORMAL_MODE;
                    break;
                }
                case '\b':
                case '\x7f': {
                    if (cx > 0) {
                        Row *row = &buf.rows[cy];
                        row_delete_char(row, cx);
                        cx--;
                    }
                    break;
                }
                default: {
                    // insert actual character
                    Row *row = &buf.rows[cy];
                    row->chars = realloc(row->chars, row->len + 2); // +1 for new char, +1 for '\0'
                    memmove(&row->chars[cx + 1], &row->chars[cx], row->len - cx + 1); // shift right (includes '\0')
                    row->chars[cx] = c;
                    row->len++;
                    cx++;
                    break;
                }
            }
        }
    }
}
