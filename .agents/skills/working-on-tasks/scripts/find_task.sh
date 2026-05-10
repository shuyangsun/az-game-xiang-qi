#!/bin/bash
# find_task.sh

if [[ -z "${1:-}" || -z "${2:-}" ]]; then
    echo "Usage: $0 SEARCH_DIR TASK-***" >&2
    exit 1
fi


SEARCH_DIR="$1"
TASK_STRING="$2"

# Prepend 'TASK-' if TASK_STRING does not start with it
if [[ "$TASK_STRING" != TASK-* ]]; then
    TASK_STRING="TASK-$TASK_STRING"
fi

OUTPUT=$(grep -rn "TODO($TASK_STRING)" --exclude='tasks.md' --include='*' "$SEARCH_DIR")

if [[ -z "$OUTPUT" ]]; then
    echo "'$TASK_STRING' not found"
else
    echo "$OUTPUT"
fi
