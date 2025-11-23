#!/bin/bash

../flimmaker test_01.mp4 --flim test_01.flim

gunzip -c test_01.flim.gz > test_01.flim.decompressed
if cmp -s test_01.flim test_01.flim.decompressed; then
	echo "TEST PASSED"
else
	echo "TEST FAILED"
fi
rm -f test_01.flim.decompressed
rm -f test_01.flim
