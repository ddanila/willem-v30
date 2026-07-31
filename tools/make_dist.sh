#!/usr/bin/env bash
set -euo pipefail

if [[ $# != 3 ]]; then
    echo "usage: $0 WILLEM.COM source-text-dir output-dir" >&2
    exit 1
fi

program=$1
textdir=$2
out=$3

mkdir -p "$out"
cp "$program" "$out/WILLEM.COM"
if [[ -e "$out/WRITE.OK" ]]; then
    echo "refusing to package a write authorization token" >&2
    exit 1
fi

for name in README.TXT HWSETUP.TXT; do
    sed 's/$/\r/' "$textdir/$name" >"$out/$name"
done

(
    cd "$out"
    sha256sum WILLEM.COM README.TXT HWSETUP.TXT | sed 's/$/\r/' >SHA256.TXT
)

for path in "$out"/*; do
    name=$(basename "$path")
    if [[ ! $name =~ ^[A-Z0-9_]{1,8}\.[A-Z0-9]{1,3}$ ]]; then
        echo "not a DOS 8.3 uppercase filename: $name" >&2
        exit 1
    fi
done

python3 - "$out" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
for path in root.glob("*.TXT"):
    data = path.read_bytes()
    if b"\n" in data.replace(b"\r\n", b""):
        raise SystemExit(f"bare LF in {path}")
print(f"DOS distribution validated: {root}")
PY
