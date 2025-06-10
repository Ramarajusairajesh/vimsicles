#!/bin/bash

# Check if zenity is installed
if ! command -v zenity &> /dev/null; then
	echo "zenity is not installed. Please install it first."
	exit 1
fi

# Create a temporary directory for selected files
TEMP_DIR=$(mktemp -d)

# Use zenity to select files
SELECTED_FILES=$(zenity --file-selection --multiple --separator="|")

if [ -z "$SELECTED_FILES" ]; then
	echo "No files selected"
	rm -rf "$TEMP_DIR"
	exit 1
fi

# Create archive name with timestamp
ARCHIVE_NAME="shared_files_$(date +%Y%m%d_%H%M%S).tar.gz"

# Create the archive
echo "$SELECTED_FILES" | tr '|' '\n' | while read -r file; do
	if [ -e "$file" ]; then
		cp -r "$file" "$TEMP_DIR/"
	fi
done

# Create the tar.gz archive
tar -czf "$ARCHIVE_NAME" -C "$TEMP_DIR" .

# Clean up
rm -rf "$TEMP_DIR"

# Calculate MD5 hash
MD5_HASH=$(md5sum "$ARCHIVE_NAME" | awk '{print $1}')

# Output the archive name and hash
echo "$ARCHIVE_NAME|$MD5_HASH"
