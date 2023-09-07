{ gitignoreNixSrc
, stdenv
, lib
, cmake
, python3
}:

let
  # The gitignoreSource function which takes a path and filters it by applying
  # gitignore rules. The result is a filtered file tree in the nix store.
  gitignoreSource = (import gitignoreNixSrc { inherit lib; }).gitignoreSource;
  python3Toolchain = python3.withPackages (_: []);
in
stdenv.mkDerivation {
  name = "Cyberus Virtualization Integration Tests";
  src = gitignoreSource ../.;

  nativeBuildInputs = [
    cmake
    python3Toolchain # for contrib/udis86
  ];
}
