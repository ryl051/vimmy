CC = gcc

CFLAGS = -Wall -g

TARGET = a

SRCS = main.c vimmy.c
OBJS = $(SRCS:.c=.o)
HEADERS = vimmy.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f $(TARGET) $(OBJS)
