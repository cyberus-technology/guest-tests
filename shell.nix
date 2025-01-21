let
  pkgs = import ./nix/nixpkgs.nix;
  release = import ./nix/release.nix { inherit pkgs; };
  cmakeDrv = release.tests.hello-world.elf32.cmakeProj;
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
    nixfmt-rfc-style

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
