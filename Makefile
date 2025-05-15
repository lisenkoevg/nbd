obj = main

$(obj): $(obj).c
	gcc $^ -o $@ -lzstd
