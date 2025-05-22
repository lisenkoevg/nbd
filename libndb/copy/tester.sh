#!/bin/bash -i

make -q || { make && sudo make install; }
../../nbdkit/filters/unzstd/tester.sh $*
