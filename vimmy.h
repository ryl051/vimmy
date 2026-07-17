#define CLEAR_SCREEN "\x1b[2J"

#include <stddef.h>
#include <sys/ioctl.h>
#include <termios.h>

typedef struct {
    char *chars;
    int len;
} Row;

typedef struct {
    Row *rows;
    int num_rows;
} Buffer;

typedef enum {
    NORMAL_MODE,
    INSERT_MODE
} Mode;

/* globals shared between vimmy.c and main.c */
extern struct winsize window_size;
extern struct termios orig_termios;
extern int cx, cy;
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

int getWindowSize(void);

/*
 * This func is written by ai. essentially reads a file and expands array if we
 * have too much data; like dynamic array
 */
void openFile(Buffer *buf, const char *filename);

/*
 * used for backspace in insert mode
 */
void row_delete_char(Row *row, int cx);

/*
 * used for dd in normal mode
 */
void buffer_delete_row(Buffer *buf, int cy);

/*
 * used for enter in insert mode
 */
<<<<<<< Updated upstream
void buffer_insert_row(Buffer *buf, int at_idx, char* text, size_t len);
=======
void buffer_insert_row(Buffer *buf, uint32_t at_idx, char* text, size_t len);

/*
 * used to save file with :w
 */

void saveFile(Buffer *buf);
>>>>>>> Stashed changes
