#!/bin/bash

print_usage() {
    echo "Usage: ./run_rlslp.sh <input_file> <queries_file> <output_file> [block_size]"
    echo
    echo "<input_file>   : Path to the input text file passed to text_rlslp_script.sh"
    echo "<queries_file> : Path to the query file passed to recomp_query"
    echo "<output_file>  : Path to the output file passed to recomp_query"
    echo "[block_size]   : Optional block size passed to text_rlslp_script.sh"
}

resolve_existing_path() {
    local path="$1"
    local dir
    local base

    dir="$(cd "$(dirname "$path")" && pwd)"
    base="$(basename "$path")"
    echo "$dir/$base"
}

resolve_output_path() {
    local path="$1"
    local dir
    local base

    dir="$(cd "$(dirname "$path")" && pwd)"
    base="$(basename "$path")"
    echo "$dir/$base"
}

if [ "$#" -lt 3 ] || [ "$#" -gt 4 ]; then
    print_usage
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FAST_RECOMP_DIR="$REPO_ROOT/fast-recompression"
INPUT_FILE="$(resolve_existing_path "$1")"
RLSLP_FILE="${INPUT_FILE}.rlslp"
QUERY_FILE="$(resolve_existing_path "$2")"
OUTPUT_FILE="$(resolve_output_path "$3")"
BLOCK_SIZE="${4:-50}"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Input file not found: $INPUT_FILE"
    exit 1
fi

if [ ! -f "$QUERY_FILE" ]; then
    echo "Query file not found: $QUERY_FILE"
    exit 1
fi

if [ ! -d "$FAST_RECOMP_DIR" ]; then
    echo "fast-recompression directory not found: $FAST_RECOMP_DIR"
    exit 1
fi

if [ ! -x "$REPO_ROOT/recomp_query" ]; then
    echo "recomp_query binary not found or not executable: $REPO_ROOT/recomp_query"
    exit 1
fi

(
    cd "$FAST_RECOMP_DIR"
    bash ./text_rlslp_script.sh "$INPUT_FILE" "$BLOCK_SIZE" >/dev/null 2>&1
)

if [ ! -f "$RLSLP_FILE" ]; then
    echo "Expected RLSLP file was not created: $RLSLP_FILE"
    exit 1
fi

"$REPO_ROOT/recomp_query" -i "$RLSLP_FILE" -qf "$QUERY_FILE" -o "$OUTPUT_FILE"
