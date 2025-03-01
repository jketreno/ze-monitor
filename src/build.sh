#!/bin/bash
set -o pipefail
build() {
    echo "Building $(date)" | tee "${ERROR_FILE}"
    if make 2>&1 | tee -a "${ERROR_FILE}"; then
        echo "Build successful" | tee -a "${ERROR_FILE}"
    else
        echo "Build failed" | tee -a "${ERROR_FILE}"
    fi
}

# Directory to watch
WATCH_DIR="."
ERROR_FILE="build.err"
if [[ -e "${ERROR_FILE}" ]]; then
    rm "${ERROR_FILE}"
fi

build

# Use inotifywait to monitor the directory for changes in *.cpp, *.hpp, and Makefile
inotifywait -m -r -e modify,create,delete --exclude '.*~$' --format '%w%f' \
  "$WATCH_DIR" | while read file; do
    # If a Makefile, .cpp or .hpp file changes, run make
    if [[ "$file" =~ \.(cpp|hpp|Makefile)$ ]]; then
        echo "Detected change in $file, running 'make'..."
        build
    fi
done
