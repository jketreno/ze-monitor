#!/bin/bash
version=$1

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
  if ! cd src; then
     fail "cd src"
  fi
  echo "$*" > version.txt
  if ! make deb; then
     fail "Unable to make deb"
  fi
  if ! git commit -a -m "version ${version}"; then
    fail "git commit -a -m 'version ${version}'"
  fi
  cmd="git tag -a v${version} -m 'Version ${version}'"
  if ! ${cmd}; then
    fail "${cmd}"
  fi
  if ask "Do you want to publish release"; then
    cmd="git push origin v${version}"
    if ! ${cmd}; then
      fail "${cmd}"
    fi
    echo "WIP"
  fi
fi

echo "Exiting."
