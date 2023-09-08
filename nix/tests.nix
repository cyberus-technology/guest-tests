pkgs:

let
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

  cmakeProj = pkgs.callPackage ./build { };

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
