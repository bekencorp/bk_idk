CC ?= gcc

CFLAGS = -g -O2 -Wall -fpic
ARFLAGS = -rc

LIB = libbk_foo.a
OBJ = foo.o

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libfoo.a: $(OBJ)
	ar $(ARFLAGS) -o $(LIB) $(OBJ)
