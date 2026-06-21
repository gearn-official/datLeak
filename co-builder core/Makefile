# co-builder-core/Makefile
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

SRCS = core_joint.c lexer.c parser.c codegen.c
OBJS = $(SRCS:.c=.o)

all: co-builder

co-builder: $(OBJS)
	$(CC) $(CFLAGS) -o co-builder $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) co-builder
