CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Wwrite-strings -Wstack-usage=1024 -Wfloat-equal -Waggregate-return -Winline

all: ashti

ashti: ashti.c
	$(CC) $(CFLAGS) ashti.c -o ashti


debug: CFLAGS += -g
debug: all

profile: CFLAGS += -pg
profile: all

clean:
	rm -f ashti 
