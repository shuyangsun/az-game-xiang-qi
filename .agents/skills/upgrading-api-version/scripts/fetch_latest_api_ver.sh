#!/bin/bash
# fetch_latest_api_ver.sh

CURL_ARGS=(-sS -L -H "Accept: application/vnd.github+json" -H "X-GitHub-Api-Version: 2022-11-28")
if [ -n "${GITHUB_TOKEN:-}" ]; then
    CURL_ARGS+=(-H "Authorization: Bearer $GITHUB_TOKEN")
fi

curl "${CURL_ARGS[@]}" \
  https://api.github.com/repos/shuyangsun/alpha-zero-api/tags \
| jq -r '
  [.[].name
   | select(test("^v[0-9]+\\.[0-9]+\\.[0-9]+$"))
   | {raw: ., key: (ltrimstr("v") | split(".") | map(tonumber))}]
  | sort_by(.key)
  | last
  | .raw
'
