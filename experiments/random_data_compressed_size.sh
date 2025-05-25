#!/bin/bash

MAX=4096
for (( i = 0; i <= MAX; i += 16)); do
  compressed_size=$(dd if=/dev/random bs=1 count=$i status=none | zstd | wc -c)
  echo "compress($i) = $compressed_size"
done
