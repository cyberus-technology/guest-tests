{ stdenv
, nix-gitignore
, cmake
}:
stdenv.mkDerivation {
  pname = "guest-tests";
  version = "1.0.0";

  src = nix-gitignore.gitignoreSourcePure [ ] ../src;

  nativeBuildInputs = [
    cmake
  ];

  # The ELFs are standalone kernels and don't need to go through these. This
  # reduces the number of warnings that scroll by during build.
  dontPatchELF = true;
  dontFixup = true;
}
