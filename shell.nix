let
  pkgs = import ./nix/cbspkgs.nix;
  cmakeDrv = pkgs.cyberus.guest-tests.tests.hello-world.elf32.cmakeProj;
  release = import ./nix/release.nix { inherit pkgs; };
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

  # Automatically install pre-commit hooks for style issues.
  shellHook = ''
    ${release.pre-commit-check.shellHook}
  '';
}
