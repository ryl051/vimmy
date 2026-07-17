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

// for command mode
char *filename = NULL;
char cmd_buf[80];
uint32_t cmd_len = 0;

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

<<<<<<< Updated upstream
    char cursorBuf[32];
    int len =
        snprintf(cursorBuf, sizeof(cursorBuf), "\x1b[%d;%dH", cy + 1, cx + 1);
    write(STDOUT_FILENO, cursorBuf, len);
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
=======
	// for command mode, draw the bottom line
	if (mode == COMMAND_MODE) {
		char status[120];
		uint32_t len = snprintf(status, sizeof(status), ":%s", cmd_buf);
		write(STDOUT_FILENO, status, len);
	} else {
		char* status;
		if (mode == NORMAL_MODE) {
			status = "-- INSERT --";
		} else {
			status = "-- NORMAL --";
		}
		write(STDOUT_FILENO, status, strlen(status));
	}
	write(STDOUT_FILENO, "\x1b[K", 3); // clear rest of the bottom line...?

	// position cursor
	char cursorBuf[32];
	uint32_t len;
	if (mode == COMMAND_MODE) {
		len = snprintf(cursorBuf, sizeof(cursorBuf), "\x1b[%d;%dH", window_size.ws_row, cmd_len + 2);
	} else {
		len = snprintf(cursorBuf, sizeof(cursorBuf), "\x1b[%d;%dH", cy + 1, cx + 1);
	}

	write(STDOUT_FILENO, cursorBuf, len);
	write(STDOUT_FILENO, "\x1b[?25h", 6);
>>>>>>> Stashed changes
}

int getWindowSize() {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) != 0) {
        perror("ioctl failed!");
        return 1;
    }
    return 0;
}

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

void buffer_insert_row(Buffer *buf, int at_idx, char* text, size_t len) {
	if (at_idx < 0 || at_idx > buf->num_rows) {
		return;
	}

	// Grow the array of Row structs by 1
	buf->rows = realloc(buf->rows, sizeof(Row) * (buf->num_rows + 1));

	// Shift all rows below at_idx down by one to open up a gap
	int num_rows_to_move = buf->num_rows - at_idx;
	if (num_rows_to_move > 0) {
		memmove(&buf->rows[at_idx + 1], &buf->rows[at_idx], sizeof(Row) * num_rows_to_move);
	}
	
	// init new row
	buf->rows[at_idx].len = len;
	buf->rows[at_idx].chars = malloc(len + 1);
	memcpy(buf->rows[at_idx].chars, text, len);
	buf->rows[at_idx].chars[len] = '\0';

	buf->num_rows++;
}

void saveFile(Buffer *buf) {
	if (filename == NULL) {
		// prompt filename here in the future?
		return;
	}

	FILE *fp = fopen(filename, "w");
	if (!fp) {
		perror("Failed to open file for writing!");
		return;
	}

	for (uint32_t i = 0; i < buf->num_rows; i++) {
		fprintf(fp, "%s\n", buf->rows[i].chars);
	}

	fclose(fp);
}
