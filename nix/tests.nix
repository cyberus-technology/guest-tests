{ gitignoreNixSrc
, stdenv
, lib
, cmake
, python3
, pkgs
}:

let
  src = gitignoreSource ../src;
  # The gitignoreSource function which takes a path and filters it by applying
  # gitignore rules. The result is a filtered file tree in the nix store.
  gitignoreSource = (import gitignoreNixSrc { inherit lib; }).gitignoreSource;
  python3Toolchain = python3.withPackages (_: [ ]);

  testNames = builtins.map (s: "${s}_guesttest-elf32") [
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
  };

  extractTest = name: stdenv.mkDerivation {
    inherit src;
    name = "Cyberus Virtualization Integration Tests - ${name}";

    doCheck = false;
    doBuild = false;
    doInstall = true;

    installPhase = ''
      mkdir -p $out
      cp ${cmakeProj}/tests/${name} $out/${name}
    '';
  };

  individualTests = builtins.foldl'
    (acc: name: acc // { "${name}" = extractTest name; })
    { }
    testNames;
in
{
  all = cmakeProj;
} // individualTests
