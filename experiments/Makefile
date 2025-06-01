obj = main
LIB = -lzstd
CFLAGS = -Wall -pedantic

all: main zstd_compress_bound dump_buffer myzstd

$(obj): $(obj).c
	gcc $^ -o $@ $(LIB)

zstd_compress_bound: zstd_compress_bound.c
	gcc $^ -o $@ $(LIB)

dump_buffer: dump_buffer.c
	gcc -DTEST_DUMP_BUFFER $^ -o $@

myzstd: myzstd.c read_input.h
	gcc $(CFLAGS) $< -o $@ $(LIB)
