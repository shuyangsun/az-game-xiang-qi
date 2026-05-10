#!/bin/bash
# next_task.sh
#
# Print the next incomplete task from tasks.md without loading the full task
# table into the agent's context window. Output format:
#
#   === NEXT TASK SNIPPET (...) ===
#   <header row>
#   <separator row>
#   ...
#   <line before target>
#   <target task row>
#   <line after target>
#   ...
#   === END SNIPPET ===
#
# The header gives the LLM the column names; the "..." markers signal that
# other task rows exist but are intentionally omitted.

set -euo pipefail

if [[ -z "${1:-}" ]]; then
    echo "Usage: $0 TASKS_MD_PATH" >&2
    exit 1
fi

TASKS_FILE="$1"

if [[ ! -f "$TASKS_FILE" ]]; then
    echo "Error: file not found: $TASKS_FILE" >&2
    exit 1
fi

# Find the markdown table separator row (e.g. "| ----- | ----- |").
SEP_LINE=$(grep -nE '^\| *-+' "$TASKS_FILE" | head -n 1 | cut -d: -f1 || true)

if [[ -z "$SEP_LINE" ]]; then
    echo "Error: could not find markdown table header in $TASKS_FILE" >&2
    exit 1
fi

HEADER_LINE=$((SEP_LINE - 1))

# Find the first incomplete task row (Completed cell == "[ ]").
LINE_NUM=$(grep -nE '\| *\[ \] *\|' "$TASKS_FILE" | head -n 1 | cut -d: -f1 || true)

if [[ -z "$LINE_NUM" ]]; then
    echo "All tasks complete — no incomplete task found in $TASKS_FILE."
    exit 0
fi

START=$(( LINE_NUM - 1 ))
END=$(( LINE_NUM + 1 ))

# Skip overlap with the header block if the target is near the top.
if [[ $START -le $SEP_LINE ]]; then
    START=$((SEP_LINE + 1))
fi

print_range() {
    awk -v s="$1" -v e="$2" 'NR>=s && NR<=e { printf "%d: %s\n", NR, $0 }' "$TASKS_FILE"
}

echo "=== NEXT TASK SNIPPET ($TASKS_FILE, target line $LINE_NUM) ==="
print_range "$HEADER_LINE" "$SEP_LINE"
echo "..."
print_range "$START" "$END"
echo "..."
echo "=== END SNIPPET (do NOT read the full tasks.md) ==="
