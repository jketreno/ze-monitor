#!/bin/bash
version="${1}"

echo "Current version: $(cat src/version.txt)"
echo "Updating to version: ${version}"

usage() {
cat << EOF
usage: $0 VERSION
EOF
}

if [[ "${version}" == "" ]]; then
  usage
  exit 1
fi

ask() {
    local prompt="$1"
    local response

    while true; do
        # Print the prompt and read user input
        echo -n "$prompt [y/n]: "
        read -r response

        # Convert to lowercase
        response=$(echo "$response" | tr '[:upper:]' '[:lower:]')

        # Check the response
        case "$response" in
            y|yes)
                return 0  # Return success (true)
                ;;
            n|no)
                return 1  # Return failure (false)
                ;;
            *)
                echo "Please answer with 'y' or 'n'"
                ;;
        esac
    done
}

fail() {
  echo "$*" >&2
  exit 1
}

cwd=$(pwd)
if ask "Do you want to continue"; then
  cmd=(cd build)
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  echo "$*" > ../src/version.txt
  cmd=(cmake ..)
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  cmd=(make)
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  cmd=(cpack)
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  cmd=(git commit -a -m "version ${version}")
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  cmd=(git tag -a "v${version}" -m "Version ${version}")
  echo "Running: ${cmd[*]}"
  if ! "${cmd[@]}"; then
    fail "${cmd[@]}"
  fi
  if ask "Do you want to publish release"; then
    cmd=(git push origin "v${version}")
    echo "Running: ${cmd[*]}"
    if ! "${cmd[@]}"; then
      fail "${cmd[@]}"
    fi
    echo "WIP: TODO -- cp .deb to releases"
  fi
fi

echo "Exiting."
