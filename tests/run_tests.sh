#!/bin/bash

set -e  # Exit on any error

../flimmaker test_01.mp4 --flim test_01.flim

gunzip -c test_01.flim.gz > test_01.flim.decompressed
if cmp -s test_01.flim test_01.flim.decompressed; then
	echo "TEST PASSED"
	rm -f test_01.flim.decompressed
	rm -f test_01.flim
	exit 0
else
	echo "TEST FAILED"
	rm -f test_01.flim.decompressed
	rm -f test_01.flim
	exit 1
fi
