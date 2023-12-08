# This top-level CMakeLists.txt helps the CLion IDE and other tooling to
# properly index the project. It is not part of the Nix closure.

cmake_minimum_required(VERSION 3.20)
project(cyberus-guest-tests CXX C ASM)
add_subdirectory(src)
