obj = main
LIB = -lzstd
CFLAGS = -Wall -pedantic

EXE = main zstd_compress_bound dbg myzstd pre_if

all: ${EXE}

$(obj): $(obj).c
	gcc $^ -o $@ $(LIB)

zstd_compress_bound: zstd_compress_bound.c
	gcc $^ -o $@ $(LIB)

dbg: dbg.c dbg.h
	gcc -DTEST_DBG -DDBG -DNBDCOPY_H $^ -o $@

myzstd: myzstd.c read_input.h
	gcc $(CFLAGS) $< -o $@ $(LIB)

clean:
	@rm -rf ${EXE} $(addsuffix .exe,${EXE})
