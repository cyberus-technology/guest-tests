/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

{ catch2_3
, cmake
, nix-gitignore
, pkgs-23-11
, stdenv
, wrapBintoolsWith
, wrapCCWith
}:
let
  # With nixpkgs 24.05, our unit tests fails during runtime with a segmentation
  # fault during test run initialization, which has nothing to do with the
  # actual unit test.
  # In nixpkgs 24.05, the glibc version 2.39 is introduced which does not
  # work with catch2 version 3 thus we downgrade the glibc version to
  # 2.38 (from nixpkgs 23.11) again to keep our code compiling
  patchedGcc = wrapCCWith
    {
      cc = pkgs-23-11.gcc-unwrapped;
      libc = pkgs-23-11.glibc;
      bintools = wrapBintoolsWith {
        bintools = pkgs-23-11.bintools-unwrapped;
        libc = pkgs-23-11.glibc;
      };
    };
in
stdenv.mkDerivation {
  pname = "cyberus-guest-tests";
  version = "0.0.0-dev";

  src = nix-gitignore.gitignoreSourcePure [
    # If you're testing around with the test properties, it is handy to add
    # this exclude to see quicker results. Otherwise, the whole CMake
    # project needs a rebuild.
    # "tests/**/properties.toml"
  ] ../../src;

  doCheck = true;
  checkInputs = [ catch2_3 ];

  nativeBuildInputs = [
    cmake
    patchedGcc # keep in sync with CI file
  ];

  # The ELFs are standalone kernels and don't need to go through these. This
  # reduces the number of warnings that scroll by during build.
  dontPatchELF = true;
  dontFixup = true;
}
