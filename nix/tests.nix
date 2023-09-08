{ stdenv
, runCommand
, lib
, cmake
, python3
, gitignoreNixSrc
}:

let
  src = gitignoreSource ../src;
  # The gitignoreSource function which takes a path and filters it by applying
  # gitignore rules. The result is a filtered file tree in the nix store.
  gitignoreSource = (import gitignoreNixSrc { inherit lib; }).gitignoreSource;
  python3Toolchain = python3.withPackages (_: [ ]);

  testNames = [
    "cpuid"
    "cxx"
    "debugport"
    "emulator"
    "emulator-syscall"
    "exceptions"
    "fpu"
    "lapic-modes"
    "lapic-priority"
    "lapic-timer"
    "msr"
    "pagefaults"
    "sgx"
    "sgx-launch-control"
    "tsc"
    "vmx"
  ];

  cmakeProj = stdenv.mkDerivation {
    inherit src;
    name = "Cyberus Virtualization Integration Tests";

    nativeBuildInputs = [
      cmake
      python3Toolchain # for contrib/udis86
    ];

    # The ELFs are standalone kernels and don't need to go through these. This
    # reduces the number of warnings that scroll by during build.
    dontPatchELF = true;
    dontFixup = true;
  };

  extractTest = name: runCommand "guesttest-${name}" { } ''
    mkdir -p $out
    cp ${cmakeProj}/tests/${name}_guesttest $out
    cp ${cmakeProj}/tests/${name}_guesttest-elf32 $out
  '';

  individualTests = builtins.foldl'
    (acc: name: acc // { "${name}" = extractTest name; })
    { }
    testNames;
in
{
  all = cmakeProj;
} // individualTests
