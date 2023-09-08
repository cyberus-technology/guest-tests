final: prev:

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

  all = final.cyberus.guest-tests.tests.all;

  extractTest = name: final.runCommand "guesttest-${name}" { } ''
    mkdir -p $out
    cp ${all}/tests/${name}_guesttest $out
    cp ${all}/tests/${name}_guesttest-elf32 $out
  '';

in
{
  tests = {
    all = final.callPackage ./build.nix { };
  } // (builtins.foldl'
    (acc: name: acc // { "${name}" = extractTest name; })
    { }
    testNames);
}
