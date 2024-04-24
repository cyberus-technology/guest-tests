#!/usr/bin/env bash

# Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later


# @describe Guest Test Repo Formatter
# This script applies the following formatting tools onto the repo:
# - clang-format
# - cmake-format
#
# Changes are applied in-place.

set -euo pipefail
IFS=$'\n\t'

# Ensure that this script is always executed from the root of the project.
DIR=$(dirname "$(realpath "$0")")
cd "$DIR/.." || exit

function fn_format_sources() {
    echo "Formatting Sources (C, CPP)"

    FOLDERS=(src/lib src/tests) # don't reformat contrib folder

    EXTENSIONS=(c h cpp hpp) # lds/S explicitly not listed; formatting errors
    EXTENSIONS_ARG_STR=()
    for EXTENSION in "${EXTENSIONS[@]}"; do
        EXTENSIONS_ARG_STR+=(--extension "$EXTENSION")
    done

    for FOLDER in "${FOLDERS[@]}"; do
        echo "  Processing folder: $FOLDER"
        fd --type file "${EXTENSIONS_ARG_STR[@]}" . "$FOLDER" | xargs -I {} clang-format --Werror --i {}
    done
}

function fn_format_cmake() {
    echo "Formatting CMake files"

    fd ".*\.cmake|CMakeLists.txt" | xargs -I {} cmake-format --in-place {}
}
function fn_main() {
    fn_format_cmake
    fn_format_sources
}

fn_main
