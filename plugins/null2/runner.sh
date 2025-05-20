#!/bin/bash -i

sample_size=${1:-1}
sample_data=$(echo -n $(yes '\x1' | head -n $sample_size) | sed -E 's/\s+//g')

make -s > /dev/null \
  && sudo make install >/dev/null \
  && nbdkit -D null2.flag=1 -P nbdkit.pid null2 $sample_size $V \
  && printf "$sample_data" | nbdcopy - nbd://localhost \
  ; kill_nbdkit nbdkit.pid
