let
  pkgs = import ./nix/cbspkgs.nix;
  cmakeDrv = pkgs.cyberus.guest-tests.tests.hello-world.elf32.cmakeProj;
in
pkgs.mkShell rec {
  inputsFrom = [
    # Import deps from the CMake derivation.
    cmakeDrv
  ];

  packages = with pkgs; [
    argc
    fd
    niv

    # Not strictly needed, but some developers prefer it to make.
    ninja

    clang-tools
    cmake-format
  ];
}
