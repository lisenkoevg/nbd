#!/bin/bash -i

sample_size=${1:-1}
koef=${2:-14}
# sample_data=$(echo -n $(yes '\x1' | head -n $sample_size) | sed -E 's/\s+//g')
dd if=/dev/random of=sample bs=1 count=$sample_size status=none
file_size=$((sample_size * koef))

make -s > /dev/null \
  && sudo make install >/dev/null \
  && zstd < sample > sample.zt \
  && dd if=/dev/zero of=file.img bs=1 count=$file_size status=none \
  && nbdkit $V1 -P nbdkit.pid --filter=log -D unzstd.flag=1 --filter=unzstd file file.img logfile=nbdkit.log \
  && nbdcopy $V2 - nbd://localhost < sample.zt \
  && diff -qs <(head -c $sample_size file.img) sample \
  ; kill_nbdkit nbdkit.pid
