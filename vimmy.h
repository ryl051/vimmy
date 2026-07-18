#define CLEAR_SCREEN "\x1b[2J"

#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <termios.h>

typedef struct {
    char *chars;
    uint32_t len;
} Row;

typedef struct {
    Row *rows;
    uint32_t num_rows;
} Buffer;

typedef enum {
    NORMAL_MODE,
    INSERT_MODE,
	COMMAND_MODE
} Mode;

/* globals shared between vimmy.c and main.c */
extern char* filename;
extern char cmd_buf[80]; 			// buffer to hold what is typed after : in a command
extern uint32_t cmd_len;			// length of curr command

extern struct winsize window_size;
extern struct termios orig_termios;
extern uint32_t cx, cy;
extern Mode mode;

/* helper functions defined in vimmy.c */

// disable raw mode for terminal (the normal mode in terminals)
void disableRawMode(void);

// enable raw mode for terminal (the mode where you can directly typing in letters)
void enableRawMode(void);

/*
 * this function redraws the entire screen after a keypress. without this,
 * we wouldn't see the cursor move, and the text updates wouldn't appear on the
 * actual screen.
 */
void refreshScreen(Buffer *buf);

int32_t getWindowSize(void);

/*
 * This func is written by ai. essentially reads a file and expands array if we
 * have too much data; like dynamic array
 */
void openFile(Buffer *buf, const char *filename);

/*
 * used for backspace in insert mode
 */
void row_delete_char(Row *row, uint32_t cx);

/*
 * used for dd in normal mode
 */
void buffer_delete_row(Buffer *buf, uint32_t cy);

/*
 * dw
 */
void buffer_delete_word(Buffer *buf, uint32_t cx);

/*
 * used for enter in insert mode
 */
void buffer_insert_row(Buffer *buf, uint32_t at_idx, char* text, size_t len);

/*
 * used to save file with :w
 */
void saveFile(Buffer *buf);
