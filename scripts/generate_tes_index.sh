#!/usr/bin/env bash
set -euo pipefail
OUT=data/www/tesresources.json
rm -f "$OUT"
mkdir -p $(dirname "$OUT")

echo "Generating tesresources index at $OUT"

files=()
for f in tesresources/*; do
  if [ -f "$f" ]; then
    files+=("$(basename "$f")")
  fi
done

jq -n --argjson arr "$(printf '%s
' "${files[@]}" | jq -R -s -c 'split("\n")[:-1]')" '$arr' > "$OUT"

echo "Wrote $(jq length $OUT) entries to $OUT"