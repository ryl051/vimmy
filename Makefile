CC = gcc

CFLAGS = -Wall -g

TARGET = a

all: $(TARGET)

$(TARGET): vimmy.c
	$(CC) $(CFLAGS) -o $(TARGET) vimmy.c

clean: 
	rm -r $(TARGET)
