cmake_minimum_required(VERSION 3.14)
project(ze-monitor VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -Werror -Wall -Wextra -Wpedantic -Wcast-align=strict -Wformat-security")

# Read the full version string from version.txt
file(READ "${CMAKE_SOURCE_DIR}/src/version.txt" FULL_VERSION)
string(STRIP "${FULL_VERSION}" FULL_VERSION)

# Split the version and release
string(REGEX MATCH "^([0-9.]+)-([0-9]+)$" VERSION_MATCH ${FULL_VERSION})
set(CPACK_PACKAGE_VERSION ${CMAKE_MATCH_1})
set(CPACK_PACKAGE_RELEASE ${CMAKE_MATCH_2})
add_definitions(-DAPP_VERSION="${FULL_VERSION}")

message(STATUS "Package Version: ${CPACK_PACKAGE_VERSION}")
message(STATUS "Package Release: ${CPACK_PACKAGE_RELEASE}")
message(STATUS "APP_VERSION    : ${FULL_VERSION}")

# Source files
set(SOURCES
    src/ze-monitor.cpp
    src/helpers.cpp
    src/args.cpp
    src/engine.cpp
    src/process.cpp
    src/temperature.cpp
    src/device.cpp
    src/power_domain.cpp
    src/psu.cpp
)

# Executable
add_executable(ze-monitor ${SOURCES})

# Find FTXUI using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FTXUI REQUIRED ftxui)
message(STATUS "FTXUI_INCLUDE_DIRS: ${FTXUI_INCLUDE_DIRS}")
message(STATUS "FTXUI_LIBRARIES: ${FTXUI_LIBRARIES}")

# Include directories and link libraries
include_directories(${FTXUI_INCLUDE_DIRS})
target_link_libraries(ze-monitor ${FTXUI_LIBRARIES} ze_loader)

# Installation
install(TARGETS ze-monitor DESTINATION /usr/bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/doc/ze-monitor.1 DESTINATION /usr/share/man/man1)

# Add post-install script for CPack to set capabilities and handle group creation
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst")

# Create the needed script files
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst 
"#!/bin/sh
set -e

# Create ze-monitor group if it doesn't exist
if ! getent group ze-monitor > /dev/null; then
    groupadd -r ze-monitor
    echo 'Created ze-monitor group'
fi

# Set group ownership and permissions
chgrp ze-monitor /usr/bin/ze-monitor
chmod 750 /usr/bin/ze-monitor

# Set capabilities
setcap cap_perfmon,cap_dac_read_search,cap_sys_ptrace=ep /usr/bin/ze-monitor

# Update man page database
if command -v mandb >/dev/null; then
    mandb -q
fi

exit 0
")

# Set executable permissions for the scripts
execute_process(COMMAND chmod 755 ${CMAKE_CURRENT_SOURCE_DIR}/debian/postinst)

# Enable testing
enable_testing()

# Packaging (Deb only)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "ze-monitor")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "James Ketrenos <james_zemonitor@ketrenos.com>")

# Set architecture to amd64 for 64-bit
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

# Description for Debian
set(CPACK_PACKAGE_DESCRIPTION "ze-monitor: Device monitor using Level Zero Sysman")

# Set dependencies
# Build-time dependencies (needed when compiling)
set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "libze-loader-dev")

# Runtime dependencies (needed when running the program)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libncurses6, libze1")

# Debian-specific package naming
set(CPACK_DEBIAN_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

# Additional CPack settings to enforce naming
set(CPACK_PACKAGE_FILE_NAME "${CPACK_DEBIAN_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_FILE_NAME_EXTRA "")

# Force DEB generator to use the specified filename
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY "any")

message(STATUS "Debian Package Filename: ${CPACK_DEBIAN_PACKAGE_FILE_NAME}")

include(CPack)