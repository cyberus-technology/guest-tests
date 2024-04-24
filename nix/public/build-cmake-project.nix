/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

{ catch2_3
, cmake
, gcc11
, nix-gitignore
, stdenv
}:

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
    gcc11 # keep in sync with CI file
  ];

  # The ELFs are standalone kernels and don't need to go through these. This
  # reduces the number of warnings that scroll by during build.
  dontPatchELF = true;
  dontFixup = true;
}
