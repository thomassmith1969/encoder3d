#!/usr/bin/env bash
set -euo pipefail
mkdir -p data/www/js/vendor

echo "Downloading vendor scripts into data/www/js/vendor..."

echo "Fetching fflate..."
curl -fSL -o data/www/js/vendor/fflate.umd.js https://cdn.jsdelivr.net/npm/fflate@0.7.4/umd/index.js

echo "Fetching NURBSCurve..."
curl -fSL -o data/www/js/vendor/NURBSCurve.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/curves/NURBSCurve.js

echo "Fetching NURBSUtils..."
curl -fSL -o data/www/js/vendor/NURBSUtils.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/curves/NURBSUtils.js

echo "Fetching FBXLoader..."
curl -fSL -o data/www/js/vendor/FBXLoader.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/loaders/FBXLoader.js

echo "Fetching FBXExporter (try multiple sources)..."
# Try two potential locations for FBXExporter
curl -fSL -o data/www/js/vendor/FBXExporter.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/exporters/FBXExporter.js || true
if [ ! -s data/www/js/vendor/FBXExporter.js ]; then
    curl -fSL -o data/www/js/vendor/FBXExporter.js https://raw.githubusercontent.com/mrdoob/three.js/master/examples/js/exporters/FBXExporter.js || true
fi

echo "Fetching GLTFLoader..."
curl -fSL -o data/www/js/vendor/GLTFLoader.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/loaders/GLTFLoader.js || true

echo "Fetching GLTFExporter..."
curl -fSL -o data/www/js/vendor/GLTFExporter.js https://cdn.jsdelivr.net/npm/three@0.128.0/examples/js/exporters/GLTFExporter.js || true

echo "Done. Files downloaded:"
ls -la data/www/js/vendor
