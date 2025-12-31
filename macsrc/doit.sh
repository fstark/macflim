#!/bin/bash

# Clean out existing source files
echo -n "Cleaning out existing source files..."
rm -f *.c *.h *.bin
echo " done."

# Extract source files from Mac disk
echo "Extracting source files from Mac disk..."
hmount "../MacFlim Source Code.dsk"
hcopy ":MacFlim Sources:Sources:*" .
humount

# hfsutils replaces spaces with underscores when copying from Mac to Unix
# Rename them back to preserve original Mac filenames
echo -n "Restoring spaces in filenames..."
for file in *_*; do
    if [ -f "$file" ]; then
        newname=$(echo "$file" | tr '_' ' ')
        if [ "$file" != "$newname" ]; then
            mv "$file" "$newname"
            # echo "  Renamed: $file -> $newname"
        fi
    fi
done
echo " done."

# Fix π character corruption in project filenames
# hfsutils mangles special Mac characters - fix known issues
echo -n "Fixing special characters in filenames..."
for file in *; do
    if [ -f "$file" ]; then
        # Replace corrupted π character sequences with proper π
        newname=$(echo "$file" | sed 's/\xB9/π/g')
        if [ "$file" != "$newname" ]; then
            mv "$file" "$newname"
        fi
    fi
done
echo " done."
