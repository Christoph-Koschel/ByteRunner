CFLAGS=-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wimplicit-fallthrough -O3 -std=c11 -pedantic

.PHONY: all
all: libbasm br basm dbasm basm2nasm image

libbasm: src/basm/libbasm.h

basm: src/basm/basm.c src/basm/libbasm.h
	$(CC) $(CFLAGS) src/basm.c src/libbasm/libbasm.h -o dump

br: src/basm/br.c src/basm/libbasm.h
	$(CC) $(CFLAGS) src/br.c src/libbasm/libbasm.h -o dump

dbasm: src/basm/dbasm.c src/basm/libbasm.h
	$(CC) $(CFLAGS) src/dbasm.c src/libbasm/libbasm.h -o dump

basm2nasm: src/basm/basm2nasm.c src/basm/libbasm.h
	$(CC) $(CFLAGS) src/basm2nasm.c src/libbasm/libbasm.h -o dump

image: src/basm/image.c src/basm/natives.c src/basm/libbasm.h
	$(CC) $(CFLAGS) src/image.c src/natives.c src/libbasm/libbasm.h -o dump