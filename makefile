CC = gcc
FILES = $(wildcard *.c)
 
all: $(FILES:.c=)
 
%: %.c
	$(CC) -o $@ $<
 
clean:
	rm -f *.o