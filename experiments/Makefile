
obj = main
LIB = -lzstd

all: main zstd_compress_bound

$(obj): $(obj).c
	gcc $^ -o $@ $(LIB)

zstd_compress_bound: zstd_compress_bound.c
	gcc $^ -o $@ $(LIB)
