#!/usr/bin/env bash
# Find test files containing a specific FR identifier.
# Usage: scripts/find_fr.sh <path/to/test/dir> <FR-identifier>
# Accepts both FR- and FR_ prefixes; searches for both variants.
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <path/to/test/dir> <FR-identifier>" >&2
  echo "Example: $0 tests/unit FR-001" >&2
  exit 1
fi

search_dir="$1"
fr_id="$2"

if [[ ! -d "$search_dir" ]]; then
  echo "Error: '$search_dir' is not a directory" >&2
  exit 1
fi

fr_pattern='^FR[-_][A-Za-z0-9]+([_-][A-Za-z0-9]+)*$'
if ! [[ "$fr_id" =~ $fr_pattern ]]; then
  echo "Error: '$fr_id' is not a valid FR identifier (expected FR-... or FR_... pattern)" >&2
  exit 1
fi

# Strip the FR- or FR_ prefix and build both search variants
fr_keyword="${fr_id#FR-}"
fr_keyword="${fr_keyword#FR_}"
fr_dash="FR-${fr_keyword}"
fr_under="FR_${fr_keyword}"

found=0
while IFS= read -r -d '' file; do
  rel="${file#"$search_dir"/}"
  if grep -qF "$fr_dash" "$file" || grep -qF "$fr_under" "$file"; then
    echo "Found in: $rel"
    grep -nF "$fr_dash" "$file" | while IFS= read -r line; do
      echo "  $line"
    done
    grep -nF "$fr_under" "$file" | while IFS= read -r line; do
      echo "  $line"
    done
    found=1
  fi
done < <(find "$search_dir" -type f -print0)

if [[ "$found" -eq 0 ]]; then
  echo "No files contained '${fr_dash}' or '${fr_under}'"
fi
