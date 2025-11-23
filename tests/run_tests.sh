#!/bin/bash

set -e  # Exit on any error

ARCH="${1:-x86_64}"  # Default to x86_64 if no argument provided

echo "Running tests for architecture: $ARCH"

# Find the binary (flimmaker or flimmaker.exe)
if [ -f ../flimmaker.exe ]; then
	FLIMMAKER=../flimmaker.exe
else
	FLIMMAKER=../flimmaker
fi

$FLIMMAKER test_01.mp4 --flim test_01.flim

# Check if we're on Windows (MSYS/MinGW) by looking for .exe
if [ -f ../flimmaker.exe ]; then
	IS_WINDOWS=true
else
	IS_WINDOWS=false
fi

if [ "$ARCH" = "arm64" ] || [ "$IS_WINDOWS" = "true" ]; then
	# On ARM64 or Windows, just verify the file was created (due to floating point differences)
	if [ -f test_01.flim ] && [ -s test_01.flim ]; then
		if [ "$IS_WINDOWS" = "true" ]; then
			echo "TEST PASSED (Windows - file generated successfully)"
		else
			echo "TEST PASSED (ARM64 - file generated successfully)"
		fi
		rm -f test_01.flim
		exit 0
	else
		if [ "$IS_WINDOWS" = "true" ]; then
			echo "TEST FAILED (Windows - file not generated or empty)"
		else
			echo "TEST FAILED (ARM64 - file not generated or empty)"
		fi
		rm -f test_01.flim
		exit 1
	fi
else
	# On x86_64 Linux/macOS, do full byte-for-byte comparison
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
