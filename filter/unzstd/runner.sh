#!/bin/bash -i

sample_size=${1:-1}
koef=${2:-14}
sample_data=$(echo -n $(yes '\x1' | head -n $sample_size) | sed -E 's/\s+//g')
file_size=$((sample_size * koef))

make -s > /dev/null \
  && sudo make install >/dev/null \
  && printf "$sample_data" > sample \
  && zstd < sample > sample.zt \
  && echo "sample:" && hd sample \
  && echo "sample.zt:" && hd sample.zt \
  && dd if=/dev/zero of=file.img bs=1 count=$file_size status=none \
  && nbdkit -P nbdkit.pid -D unzstd.flag=1 --filter=unzstd file file.img $VERBOSE \
  && nbdcopy - nbd://localhost < sample.zt \
  && echo "file.img:" && hd file.img \
  && diff -qs <(head -c $sample_size file.img) sample \
  ; kill_nbdkit nbdkit.pid
