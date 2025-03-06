#!/bin/bash
set -o pipefail
build() {
    echo "Building $(date)"
    cd build
    if cmake && make && cpack; then
        echo "Build successful"
    else
        echo "Build failed"
    fi
    cd ..
}

# Directory to watch
WATCH_DIR="."
LOG_FILE="build.log"
if [[ -e "${LOG_FILE}" ]]; then
    rm "${LOG_FILE}"
fi

build 2>&1 | tee "${LOG_FILE}"
if [[ "$1" == "" ]]; then
    exit
fi

# Use inotifywait to monitor the directory for changes in *.cpp, *.h, and Makefile
inotifywait -m -r -e modify,create,delete --exclude '.*~$' --format '%w%f' \
  "$WATCH_DIR" | while read file; do
    # If a Makefile, .cpp or .h file changes, run make
    if [[ "$file" =~ \.(cpp|h|CMakeLists.txt)$ ]]; then
        echo "Detected change in $file, running 'build'..." | tee -a "${LOG_FILE}"
        build 2>&1 | tee -a "${LOG_FILE}"
    fi
done
