#!/bin/bash

#
# MacFlim CI/CD Build Script
#
# This script automates the compilation of MacFlim projects on a vintage Mac
# using Mini vMac emulator with System 7.1 and THINK C compiler.
#
# Usage:
#   ./cicd.sh              - Automated build (boots, compiles, shuts down)
#   CICD_INTERACTIVE=1 ./cicd.sh - Interactive mode (no automation, for debugging)
#

# Path to Mini vMac binary - auto-detect or use environment variable
if [ -z "$MINIVMAC" ]; then
    if [ -x "/home/fred/Development/minivmac/minivmac-se30-large" ]; then
        MINIVMAC="/home/fred/Development/minivmac/minivmac-se30-large"
    elif [ -x "../minivmac" ]; then
        MINIVMAC="../minivmac"
    elif [ -x "./minivmac" ]; then
        MINIVMAC="./minivmac"
    elif command -v minivmac &> /dev/null; then
        MINIVMAC="minivmac"
    else
        echo -e "${RED}Error: minivmac not found${NC}"
        echo "Set MINIVMAC environment variable or ensure minivmac is in PATH"
        exit 1
    fi
fi

set -e  # Exit on error
trap 'cleanup' EXIT  # Cleanup on exit

# Cleanup function
cleanup() {
    if [ ! -z "$MINIVMAC_PID" ] && kill -0 $MINIVMAC_PID 2>/dev/null; then
        echo "Cleaning up Mini vMac process..."
        kill $MINIVMAC_PID 2>/dev/null || true
    fi
}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== MacFlim CI/CD Build ===${NC}"

# Configuration
CICD_DISK="MacFlim CICD.dsk"
RELEASE_DISK="MacFlim Release Master.dsk"
BUILD_DISK="MacFlim-CICD-build.dsk"
SOURCE_DIR="../macsrc"  # Source is in parent directory
VERSION="${VERSION:-dev}"  # Get version from environment or default to 'dev'

# Check prerequisites
if [ ! -f "$CICD_DISK" ]; then
    echo -e "${RED}Error: $CICD_DISK not found${NC}"
    exit 1
fi

if [ ! -f "$RELEASE_DISK" ]; then
    echo -e "${RED}Error: $RELEASE_DISK not found${NC}"
    exit 1
fi

if [ ! -x "$MINIVMAC" ]; then
    echo -e "${RED}Error: $MINIVMAC not found or not executable${NC}"
    exit 1
fi

if ! command -v hmount &> /dev/null; then
    echo -e "${RED}Error: hfsutils (hmount/hcopy/humount) not found${NC}"
    echo "Install with: sudo apt-get install hfsutils (or brew install hfsutils)"
    exit 1
fi

if ! command -v xdotool &> /dev/null; then
    echo -e "${RED}Error: xdotool not found${NC}"
    echo "Install with: sudo apt-get install xdotool"
    exit 1
fi

# Clean up any previous build disk
if [ -f "$BUILD_DISK" ]; then
    echo "Removing previous build disk..."
    rm -f "$BUILD_DISK"
fi

# Step 1: Duplicate the CICD disk
echo "Duplicating CICD disk..."
cp "$CICD_DISK" "$BUILD_DISK"

# Step 2: Mount the build disk and copy source files
echo "Mounting build disk..."
hmount "$BUILD_DISK"

# Create Sources directory - hfsutils uses hmkdir
echo "Creating Sources directory..."
hmkdir ":Sources:" 2>/dev/null || echo "  (Sources directory may already exist)"

# Copy all source files
echo "Copying source files to :Sources:..."
# Copy all files from macsrc directory
# First, unmount temporarily to access host filesystem
humount

# Re-mount and copy each file
for file in "$SOURCE_DIR"/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        echo "  Copying $filename..."
        hmount "$BUILD_DISK"
        hcopy "$(pwd)/$file" ":Sources:"
        humount
    fi
done

# Mount again for the next step
hmount "$BUILD_DISK"

# Step 3: QuicKeys automation (if configured on CICD disk)
# NOTE: QuicKeys automation should be pre-configured on the CICD disk to:
#   1. Open Sources:MacFlim XCMD.π, press Cmd-1, Return, wait, Cmd-Q
#   2. Open Sources:MacFlim.π, press Cmd-1, Return, wait, Cmd-Q  
#   3. Open Sources:Mini MacFlim.π, press Cmd-1, Return, wait, Cmd-Q
#   4. Shut down
# If QuicKeys is set to run at startup, the build will be automatic.
# Otherwise, run with CICD_INTERACTIVE=1 to manually trigger builds.

if [ -z "$CICD_INTERACTIVE" ]; then
    echo -e "${GREEN}Build automation enabled${NC}"
    echo "QuicKeys should run automatically at boot (if configured)"
else
    echo -e "${YELLOW}Interactive mode - manual build required${NC}"
fi

# Unmount the disk
echo "Unmounting build disk..."
humount

# Screenshot counter for debugging
SCREENSHOT_COUNTER=1

# Helper function to send keystrokes safely
send_key() {
    local key="$1"
    local delay="${2:-0.5}"
    
    # Capture screenshot before sending key
    local screenshot_name=$(printf "%03d - %s.png" $SCREENSHOT_COUNTER "$key")
    scrot -u -w $WINDOW_ID "debug-$screenshot_name" 2>/dev/null || \
        import -window $WINDOW_ID "debug-$screenshot_name" 2>/dev/null || \
        echo "  (Screenshot failed)"
    SCREENSHOT_COUNTER=$((SCREENSHOT_COUNTER + 1))
    
    # Try to ensure window has focus (may fail in xvfb, but that's OK)
    xdotool windowactivate $WINDOW_ID 2>/dev/null || true
    
    # Small delay to ensure activation
    sleep 0.2
    
    # Send the keystroke directly to window (works even without activation)
    xdotool key --window $WINDOW_ID "$key" || {
        echo -e "${RED}Error: Failed to send key $key${NC}"
        return 1
    }
    
    sleep "$delay"
}

# Function to build one project
build_project() {
    local project_name="$1"
    local select_key="$2"
    local compile_wait="${3:-2}"
    
    echo ""
    echo -e "${GREEN}Building: $project_name${NC}"
    
    # Select project by typing its first character
    echo "  Selecting project file (typing '$select_key')..."
    send_key "$select_key" 0.5
    
    # Open project (Cmd-O = Alt-O in Mini vMac)
    echo "  Opening project..."
    send_key "alt+o" 1
       
    # Remove objects first (Cmd-1 = Alt-1) for clean build
    echo "  Removing objects (clean build)..."
    send_key "alt+1" 0.1
    
    # Confirm remove objects (Return)
    echo "  Confirming removal..."
    send_key "Return" 1
    
    # Build (Cmd-2 = Alt-2)
    echo "  Starting compilation..."
    send_key "alt+2" 0.2
    
    # Confirm build dialog (Return)
    send_key "Return" 4
    
    # Confirm output name (Return)
    echo "  Confirming output..."
    send_key "Return" "$compile_wait"
    
    # Wait for compilation to complete
    echo "  Compiling (waiting ${compile_wait}s)..."
    
    # Quit THINK C (Cmd-Q = Alt-Q)
    echo "  Quitting THINK C..."
    send_key "alt+q" 0.5
    
    echo -e "${GREEN}  ✓ $project_name build complete${NC}"
}

# Step 4: Launch Mini vMac and automate build
echo ""
echo "Launching Mini vMac..."
# Step 4: Launch Mini vMac and automate build
echo ""
echo "Launching Mini vMac..."

if [ -z "$CICD_INTERACTIVE" ]; then
    echo "Automated build mode - using xdotool to control emulator"
    
    # Launch Mini vMac in background
    "$MINIVMAC" "$BUILD_DISK" "$RELEASE_DISK" &
    MINIVMAC_PID=$!
    
    echo "Mini vMac PID: $MINIVMAC_PID"
    
    # Wait for Mini vMac window to appear
    echo "Waiting for Mini vMac window..."
    sleep 1
    
    # Find the actual Mac screen window (512x342 or 1024x768, not the chrome window)
    # Get all minivmac windows and find the one with the right geometry
    for wid in $(xdotool search --name "minivmac" 2>/dev/null); do
        geometry=$(xdotool getwindowgeometry $wid | grep Geometry | awk '{print $2}')
        if [[ "$geometry" == "512x342" ]] || [[ "$geometry" == "1024x768" ]]; then
            WINDOW_ID=$wid
            WINDOW_GEOMETRY=$geometry
            break
        fi
    done
    
    if [ -z "$WINDOW_ID" ]; then
        echo -e "${RED}Error: Failed to find Mini vMac screen window (512x342 or 1024x768)${NC}"
        exit 1
    fi
    
    echo "Mini vMac window ID: $WINDOW_ID (geometry: $WINDOW_GEOMETRY)"
    
    # Wait for System 7.1 to boot
    echo "Waiting for System 7.1 to boot (2 seconds)..."
    sleep 2
    
    # Try to activate window (may fail in xvfb, but that's OK)
    xdotool windowactivate $WINDOW_ID 2>/dev/null || echo "  (Window activation not supported in headless mode)"
    sleep 1
    
    # Open Sources folder
    echo ""
    echo "Opening Sources folder..."
    send_key "s" 0.5
    send_key "alt+o" 1
    echo "Sources folder opened"
    
    # Build first project: MacFlim (main player) - starts with '1'
    build_project "MacFlim" "1" 2
    
    # Build second project: Mini MacFlim - starts with '2'
    build_project "Mini MacFlim" "2" 2
    
    # Build third project: MacFlim XCMD - starts with '3'
    build_project "MacFlim XCMD" "3" 2
    
    # Run shutdown app - project starts with '4'
    echo ""
    echo -e "${GREEN}Running shutdown app${NC}"
    echo "  Selecting shutdown project (typing '4')..."
    send_key "4" 0.5
    
    echo "  Opening shutdown project..."
    send_key "alt+o" 1
    
    echo "  Waiting for THINK C..."
    sleep 1
    
    # Clean first (Cmd-1 = Alt-1)
    echo "  Cleaning shutdown project..."
    send_key "alt+1" 0.5
    
    # Confirm clean (Return)
    echo "  Confirming clean..."
    send_key "Return" 0.5
    
    echo "  Running shutdown app (Cmd-R)..."
    send_key "alt+r" 0.2
    
    # Confirm run dialog (Return)
    send_key "Return" 1
    
    echo "  Mac should shut down..."
    
    # Wait for Mini vMac to exit (with timeout)
    echo ""
    echo "Waiting for Mini vMac to exit..."
    
    # Wait up to 5 seconds for Mini vMac to exit
    timeout=5
    elapsed=0
    while kill -0 $MINIVMAC_PID 2>/dev/null && [ $elapsed -lt $timeout ]; do
        sleep 1
        elapsed=$((elapsed + 1))
    done
    
    if kill -0 $MINIVMAC_PID 2>/dev/null; then
        echo -e "${YELLOW}Warning: Mini vMac did not exit after ${timeout}s, forcing shutdown${NC}"
        kill $MINIVMAC_PID 2>/dev/null || true
        sleep 2
    fi
    
else
    echo -e "${YELLOW}Interactive mode - you can manually test the build${NC}"
    
    # Use the minivmac binary (not minivmac128)
    "$MINIVMAC" "$BUILD_DISK" "$RELEASE_DISK"
fi

echo ""
echo "Mini vMac exited"

# Step 5: Extract and verify binaries
if [ -z "$CICD_INTERACTIVE" ]; then
    echo ""
    echo "Extracting compiled binaries..."
    
    # Mount the build disk to check for binaries
    hmount "$BUILD_DISK"
    
    # Check if binaries exist
    MACFLIM_EXISTS=0
    MINI_EXISTS=0
    
    # Try to copy MacFlim binary
    if hcopy -m ":Sources:MacFlim" "/tmp/MacFlim.tmp" 2>/dev/null; then
        MACFLIM_EXISTS=1
        rm -f "/tmp/MacFlim.tmp"
        echo -e "${GREEN}✓ MacFlim binary found${NC}"
    else
        echo -e "${RED}✗ MacFlim binary not found${NC}"
    fi
    
    # Try to copy Mini MacFlim binary
    if hcopy -m ":Sources:Mini MacFlim" "/tmp/MiniMacFlim.tmp" 2>/dev/null; then
        MINI_EXISTS=1
        rm -f "/tmp/MiniMacFlim.tmp"
        echo -e "${GREEN}✓ Mini MacFlim binary found${NC}"
    else
        echo -e "${RED}✗ Mini MacFlim binary not found${NC}"
    fi
    
    humount
    
    if [ $MACFLIM_EXISTS -eq 1 ] && [ $MINI_EXISTS -eq 1 ]; then
        echo ""
        echo "Copying binaries to Release Master disk..."
        
        # Mount both disks to copy binaries
        hmount "$BUILD_DISK"
        
        # Copy MacFlim
        hcopy -m ":Sources:MacFlim" "./MacFlim.tmp"
        
        humount
        
        hmount "$RELEASE_DISK"
        hcopy -m "./MacFlim.tmp" ":MacFlim"
        rm -f "./MacFlim.tmp"
        
        humount
        
        # Mount again for Mini MacFlim
        hmount "$BUILD_DISK"
        hcopy -m ":Sources:Mini MacFlim" "./MiniMacFlim.tmp"
        humount
        
        hmount "$RELEASE_DISK"
        hcopy -m "./MiniMacFlim.tmp" ":Mini MacFlim"
        rm -f "./MiniMacFlim.tmp"
        
        humount
        
        echo -e "${GREEN}✓ Binaries copied to Release Master disk${NC}"
        
        # Create versioned copy of release disk
        if [ "$VERSION" != "dev" ]; then
            echo "Creating versioned release disk..."
            cp "$RELEASE_DISK" "MacFlim-v${VERSION}-Release.dsk"
            echo "Created: MacFlim-v${VERSION}-Release.dsk"
        fi
        
        # Clean up build disk
        echo "Cleaning up..."
        rm -f "$BUILD_DISK"
        
        echo ""
        echo -e "${GREEN}=== Build Successful ===${NC}"
        if [ "$VERSION" != "dev" ]; then
            echo "Release disk: MacFlim-v${VERSION}-Release.dsk"
        fi
        exit 0
    else
        echo ""
        echo -e "${RED}=== Build Failed ===${NC}"
        echo "One or more binaries were not generated"
        echo "Build disk preserved as: $BUILD_DISK"
        exit 1
    fi
else
    echo ""
    echo -e "${YELLOW}Interactive mode - skipping binary extraction${NC}"
    echo "Build disk preserved as: $BUILD_DISK"
    exit 0
fi

# TODO: Future enhancements
# - Use hattrib to set Finder icon positions for pixel-perfect layout
# - Edit version resources in binaries (may require custom tools)
# - Timestamp checking to verify fresh compilation
# - GitHub Actions integration
