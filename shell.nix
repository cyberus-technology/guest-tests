{ sources ? import ./nix/sources.nix
, pkgs ? import sources.nixpkgs { }
}:

let
  python3Toolchain = pkgs.python3.withPackages (_: [ ]);
in
pkgs.mkShell rec {
  # CLI Utilities
  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    fd
    argc
    niv

    python3Toolchain

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
