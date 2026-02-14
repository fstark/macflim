#!/bin/bash

set -e  # Exit on any error

# Detect current architecture and OS
detect_arch() {
	local os=$(uname -s)
	local arch=$(uname -m)
	
	# Normalize OS name
	case "$os" in
		Darwin)
			os="macos"
			;;
		Linux)
			os="linux"
			;;
		MINGW*|MSYS*|CYGWIN*)
			os="windows"
			;;
		*)
			os=$(echo "$os" | tr '[:upper:]' '[:lower:]')
			;;
	esac
	
	echo "${os}-${arch}"
}

ARCH=$(detect_arch)
MODE="${1:-test}"  # 'test' or '--make-reference'

# Find the binary (flimmaker or flimmaker.exe)
if [ -f ../flimmaker.exe ]; then
	FLIMMAKER=../flimmaker.exe
else
	FLIMMAKER=../flimmaker
fi

# Generate reference file mode
if [ "$MODE" = "--make-reference" ]; then
	echo "Generating reference file for architecture: $ARCH"
	
	$FLIMMAKER test_01.mp4 --flim test_01.flim
	
	if [ ! -f test_01.flim ] || [ ! -s test_01.flim ]; then
		echo "ERROR: Failed to generate test_01.flim"
		rm -f test_01.flim
		exit 1
	fi
	
	# Compress reference file with architecture-specific name
	REFERENCE_FILE="test_01-${ARCH}.flim.gz"
	gzip -c test_01.flim > "$REFERENCE_FILE"
	rm -f test_01.flim
	
	echo "Reference file created: $REFERENCE_FILE"
	exit 0
fi

# Normal test mode
echo "Running tests for architecture: $ARCH"

$FLIMMAKER test_01.mp4 --flim test_01.flim

# Check if file was generated
if [ ! -f test_01.flim ] || [ ! -s test_01.flim ]; then
	echo "TEST FAILED - file not generated or empty"
	rm -f test_01.flim
	exit 1
fi

# Look for architecture-specific reference file
REFERENCE_FILE="test_01-${ARCH}.flim.gz"

if [ -f "$REFERENCE_FILE" ]; then
	# Reference file exists - do byte-for-byte comparison
	gunzip -c "$REFERENCE_FILE" > test_01.flim.reference
	if cmp -s test_01.flim test_01.flim.reference; then
		echo "TEST PASSED ($ARCH - exact match)"
		rm -f test_01.flim.reference test_01.flim
		exit 0
	else
		echo "TEST FAILED ($ARCH - files differ)"
		rm -f test_01.flim.reference test_01.flim
		exit 1
	fi
else
	# No reference file - fallback to smoke test
	echo "WARNING: No reference file for $ARCH, using smoke test"
	echo "  (to create: make test-reference)"
	echo "TEST PASSED ($ARCH - smoke test: file generated successfully)"
	rm -f test_01.flim
	exit 0
fi
