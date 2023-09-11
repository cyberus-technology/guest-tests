let
  pkgs = import ./nix/cbspkgs.nix;
in
pkgs.mkShell rec {
  packages = with pkgs; [
    fd
    argc
    niv

    # Not strictly needed, but some developers prefer it to make.
    ninja

    clang-tools
    cmake-format
  ];

  inputsFrom = [
    pkgs.cyberus.guest-tests.tests
  ];
}
