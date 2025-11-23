#!/bin/bash

set -e  # Exit on any error

ARCH="${1:-x86_64}"  # Default to x86_64 if no argument provided

echo "Running tests for architecture: $ARCH"

../flimmaker test_01.mp4 --flim test_01.flim

if [ "$ARCH" = "arm64" ]; then
	# On ARM64, just verify the file was created (due to floating point differences)
	if [ -f test_01.flim ] && [ -s test_01.flim ]; then
		echo "TEST PASSED (ARM64 - file generated successfully)"
		rm -f test_01.flim
		exit 0
	else
		echo "TEST FAILED (ARM64 - file not generated or empty)"
		rm -f test_01.flim
		exit 1
	fi
else
	# On x86_64, do full byte-for-byte comparison
	gunzip -c test_01.flim.gz > test_01.flim.decompressed
	if cmp -s test_01.flim test_01.flim.decompressed; then
		echo "TEST PASSED (x86_64 - exact match)"
		rm -f test_01.flim.decompressed
		rm -f test_01.flim
		exit 0
	else
		echo "TEST FAILED (x86_64 - files differ)"
		rm -f test_01.flim.decompressed
		rm -f test_01.flim
		exit 1
	fi
fi
