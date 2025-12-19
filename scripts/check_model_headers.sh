#!/usr/bin/env bash
set -euo pipefail

DIR="tesresources"
printf "Checking model headers in %s\n\n" "$DIR"

for f in "$DIR"/*; do
  [ -f "$f" ] || continue
  printf "File: %s\n" "$f"
  header=$(head -c 64 "$f" 2>/dev/null | xxd -p -c 64)
  text=$(head -c 512 "$f" 2>/dev/null | tr -d '\\0' | sed -n '1,5p')
  if echo "$text" | grep -q "Kaydara FBX Binary"; then
    echo "  -> FBX (binary) detected by 'Kaydara FBX Binary'"
  elif echo "$text" | grep -q "FBXHeaderExtension"; then
    echo "  -> FBX (ASCII) detected by 'FBXHeaderExtension'"
  elif echo "$header" | grep -iq "676c7466"; then
    echo "  -> GLB detected (magic 'glTF')"
  elif echo "$text" | grep -q "^{"; then
    echo "  -> Probably GLTF JSON (starts with {)"
  elif echo "$text" | grep -qi "<html"; then
    echo "  -> Not a model file: appears to be HTML (downloaded a webpage)"
  else
    echo "  -> Unknown format. Header (hex): $header"
  fi
  echo
done
