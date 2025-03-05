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
    src/ze-ncurses.cpp
)

# Executable
add_executable(ze-monitor ${SOURCES})

# Link libraries
target_link_libraries(ze-monitor ncurses panel ze_loader)

# Installation
install(TARGETS ze-monitor DESTINATION /usr/bin)

# Enable testing
enable_testing()

# Packaging (Deb and RPM)
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_PACKAGE_NAME "ze-monitor")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "James Ketrenos <james_zemonitor@ketrenos.com>")
set(CPACK_RPM_PACKAGE_LICENSE "BSD-2")

# Set architecture to amd64 for 64-bit
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

# Description for Debian and RPM
set(CPACK_PACKAGE_DESCRIPTION "ze-monitor: Device monitor using Level Zero Sysman")

# Set dependencies
# Build-time dependencies (needed when compiling)
set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "libze-loader-dev")
set(CPACK_RPM_PACKAGE_BUILDS_REQUIRES "libze-loader-devel")

# Runtime dependencies (needed when running the program)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libncurses6, libze1")
set(CPACK_RPM_PACKAGE_REQUIRES "ncurses, ncurses-libs, libze1")


# Debian-specific package naming
set(CPACK_DEBIAN_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

# RPM package naming
set(CPACK_RPM_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")

# Additional CPack settings to enforce naming
set(CPACK_PACKAGE_FILE_NAME "${CPACK_DEBIAN_PACKAGE_FILE_NAME}")
set(CPACK_PACKAGE_FILE_NAME_EXTRA "")

# Force DEB generator to use the specified filename
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY "any")

message(STATUS "Debian Package Filename: ${CPACK_DEBIAN_PACKAGE_FILE_NAME}")
message(STATUS "RPM Package Filename: ${CPACK_RPM_FILE_NAME}")

include(CPack)
