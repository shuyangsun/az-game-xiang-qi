#!/bin/bash
# parse_current_api_ver.sh

# Default to CMakeLists.txt in the current directory, but allow an argument override
CMAKE_FILE="${1:-CMakeLists.txt}"

# Ensure the file exists before attempting to parse
if [[ ! -f "$CMAKE_FILE" ]]; then
    echo "Error: File '$CMAKE_FILE' not found." >&2
    exit 1
fi

# Extract the tag using perl:
# -0777: slurp the entire file so the regex can match across lines
# \bset\s*\(\s*: matches 'set(' (case-insensitive) with flexible spacing
# ALPHA_ZERO_API_GIT_TAG\s+: matches the variable name followed by whitespace
# "([^"]+)": captures the value inside the double quotes
GIT_TAG=$(perl -0777 -nE 'print $1 if /\bset\s*\(\s*ALPHA_ZERO_API_GIT_TAG\s+"([^"]+)"/i' "$CMAKE_FILE")

# Check if the extraction was successful
if [[ -z "$GIT_TAG" ]]; then
    echo "Error: Could not find ALPHA_ZERO_API_GIT_TAG in '$CMAKE_FILE'." >&2
    exit 1
fi

# Output the parsed tag to standard output
echo "$GIT_TAG"
