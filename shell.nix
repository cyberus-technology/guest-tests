{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell rec {
  # CLI Utilities
  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    fd
    argc

    clang-tools
    cmake-format
  ];

  # Header Files, Runtime Dependencies
  buildInputs = with pkgs; [
    gtest
  ];

  # Enable to find shared objects, such as libX11.so.
  # LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
}
