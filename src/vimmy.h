#define CLEAR_SCREEN "\x1b[2J"

#include <stddef.h>

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

void openFile(Buffer *buf, const char *filename);