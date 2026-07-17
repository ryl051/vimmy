#include "vimmy.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    enableRawMode();
    getWindowSize();

    Buffer buf = {NULL, 0};
    if (argc >= 2) {
        openFile(&buf, argv[1]);
		filename = argv[1];
	}

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
                    // wait for another input if we're trying to delete
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
				case ':': {
					mode = COMMAND_MODE;
					cmd_buf[0] = '\0';
					cmd_len = 0;
					break;
				}
            }
            // insert mode
        } else if (mode == INSERT_MODE) {
            switch (c) {
                case '\x1b': {
                    mode = NORMAL_MODE;
                    break;
                }

				// backspace
                case '\b':
                case '\x7f': {
                    if (cx > 0) {
                        Row *row = &buf.rows[cy];
                        row_delete_char(row, cx);
                        cx--;
                    }
                    break;
                }
				case '\r':
				case '\n': {
					Row *row = &buf.rows[cy];

					// split the current row at character cx
					// aka everything from cx + 1 goes onto new line
					char *split_text = &row->chars[cx];
					uint32_t split_len = row->len - cx;

					buffer_insert_row(&buf, cy + 1, split_text, split_len);

					// truncate the original row
					row = &buf.rows[cy];
					row->len = cx;
					row->chars = realloc(row->chars, row->len + 1);
					row->chars[row->len] = '\0';

					cy++;
					cx = 0;
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
		// COMMAND MODE
        } else {
			switch(c) {
				// escape to cancel the command
				case '\x1b': {
					mode = NORMAL_MODE;
					break;
				}
				case '\r':
				case '\n': {
					cmd_buf[cmd_len] = '\0';
					if (strcmp(cmd_buf, "w") == 0) {
						saveFile(&buf);
					} else if (strcmp(cmd_buf, "q") == 0) {
						disableRawMode();
						exit(0);
					}
					mode = NORMAL_MODE;
					break;
				}
				case '\b':
				case '\x7f': {
					if (cmd_len > 0) {
						cmd_len--;
						cmd_buf[cmd_len] = '\0';
					} else {
						mode = NORMAL_MODE;
					}
					break;
				}
				default: {
					// prevent buffer overflow
					if (cmd_len < (int)sizeof(cmd_buf) - 1) {
						cmd_buf[cmd_len++] = c;
						cmd_buf[cmd_len] = '\0';
					}
					break;
				}
			}
		}
    }
}
