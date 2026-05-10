#!/bin/bash
# fetch_migration_guides.sh
# Run from project root dir

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ALPHA_ZERO_API_CURRENT_VER=$("$SCRIPT_DIR/parse_current_api_ver.sh")
ALPHA_ZERO_API_LATEST_VER=$("$SCRIPT_DIR/fetch_latest_api_ver.sh")

if [ "${ALPHA_ZERO_API_CURRENT_VER}" = "${ALPHA_ZERO_API_LATEST_VER}" ]; then
    echo "Already at latest version"
    exit 0
fi

# Fetch and format the migration guides as markdown checklists
CURL_ARGS=(-sS -L -H "Accept: application/vnd.github+json" -H "X-GitHub-Api-Version: 2022-11-28")
if [ -n "${GITHUB_TOKEN:-}" ]; then
    CURL_ARGS+=(-H "Authorization: Bearer $GITHUB_TOKEN")
fi

MIGRATION_TASKS=$(curl "${CURL_ARGS[@]}" \
  https://api.github.com/repos/shuyangsun/alpha-zero-api/contents/doc/migration-guides \
| jq -r --arg curr "$ALPHA_ZERO_API_CURRENT_VER" --arg latest "$ALPHA_ZERO_API_LATEST_VER" '
  if type == "object" and .message == "Not Found" then
    "No migration guide found"
  elif type == "array" then
    def parse_ver: ltrimstr("v") | split(".") | map(tonumber);

    ($curr | parse_ver) as $c_ver |
    ($latest | parse_ver) as $l_ver |

    # Filter files, extract the html_url, and parse versions
    [
      .[] | select(.type == "file" and (.name | test("^v[0-9]+\\.[0-9]+\\.[0-9]+-to-v[0-9]+\\.[0-9]+\\.[0-9]+\\.md$"))) |
      .name as $raw |
      .html_url as $url |
      ($raw | sub("\\.md$"; "") | split("-to-")) as $parts |
      {
        from_str: $parts[0],
        to_str: $parts[1],
        from_v: ($parts[0] | parse_ver),
        to_v: ($parts[1] | parse_ver),
        url: $url
      }
    ] as $guides |

    # Find the optimal starting version
    ([ $guides[] | select(.from_v <= $c_ver) | .from_v ] | max) as $start_v |

    if $start_v == null then
      empty
    else
      # Filter guides, sort, and format as a markdown checklist
      $guides | map(
        select(.from_v >= $start_v and .to_v <= $l_ver)
      ) | sort_by(.from_v) | .[] | "- [ ] [`\(.from_str)` to `\(.to_str)`](\(.url))"
    end
  else
    empty
  end
')

# Check if tasks were found
if [ -z "$MIGRATION_TASKS" ]; then
    exit 0
elif [ "$MIGRATION_TASKS" = "No migration guide found" ]; then
    echo "No migration guide found"
    exit 0
fi

MEM_FILE="./memory/migrations.md"

# Ensure the directory and file exist before trying to modify them
mkdir -p "$(dirname "$MEM_FILE")"
touch "$MEM_FILE"

# 1. Remove extra newlines at the end only if the last non-empty line is a checklist item.
# Bash command substitution $() automatically strips all trailing newlines.
FILE_CONTENT=$(<"$MEM_FILE")

LAST_NON_EMPTY_LINE=$(echo "$FILE_CONTENT" | sed '/^[[:space:]]*$/d' | tail -n 1)

if [[ "$LAST_NON_EMPTY_LINE" == "- [ ]"* ]]; then
    # Write the content back to the file with absolutely no trailing newlines.
    echo -n "$FILE_CONTENT" > "$MEM_FILE"

    # If the file wasn't totally empty, add one standard newline so the new checklist starts on a fresh line.
    if [ -n "$FILE_CONTENT" ]; then
        echo "" >> "$MEM_FILE"
    fi

    # 2 & 3. Append the new migration checklists (echo automatically adds a standard trailing newline at the end of the block)
    echo "$MIGRATION_TASKS" >> "$MEM_FILE"

    # 4. Add back the extra newline character at the end of the file
    echo "" >> "$MEM_FILE"
else
    # Preserve the existing trailing newline(s). Add a blank line for separation, then append.
    if [ -n "$FILE_CONTENT" ]; then
        echo "" >> "$MEM_FILE"
    fi
    echo "$MIGRATION_TASKS" >> "$MEM_FILE"
fi

# 5. Print success message
echo "API migration tasks added to memory/migrations.md"
