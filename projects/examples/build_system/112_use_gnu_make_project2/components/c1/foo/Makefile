CC ?= arm-none-eabi-gcc

CFLAGS = -mcpu=cortex-m33+nodsp -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mcmse --gc-sections -nostdlib -Wl,-g -Os -nostdlib -Wall -Werror -Wno-address -Wno-array-bounds -Wno-format -Wno-unknown-pragmas -Wno-address-of-packed-member -Wno-deprecated-declarations -Wno-unused-value -Wno-unused-function -ffunction-sections -fsigned-char -fdata-sections -fno-strict-aliasing -ggdb -fstack-protector -Wno-builtin-macro-redefined -std=gnu99 -Wno-old-style-declaration 
ARFLAGS = -rc

LIB = libbk_foo.a
OBJ = foo.o

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libfoo.a: $(OBJ)
	ar $(ARFLAGS) -o $(LIB) $(OBJ)

clean:
	rm -rf *.o
