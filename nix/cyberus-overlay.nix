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

  # All tests from the CMake build.
  allTests = final.cyberus.guest-tests.tests;

  # Attribute set that maps the name of each test to a derivation that contains
  # all binary variants of that test. Each inner attribute provides the
  # individual binary variants as passthru attributes.
  testsByName = builtins.foldl'
    (acc: name: acc // { "${name}" = extractTestAllVariants name; })
    { }
    testNames;

  # Extracts all binary variants of a test from the CMake build of all tests.
  extractTestAllVariants =
    name:
    final.runCommand "guesttest-${name}-all"
      {
        passthru = testByVariant name;
      } ''
      mkdir -p $out
      cp ${allTests}/${name}_guesttest $out
      cp ${allTests}/${name}_guesttest-elf32 $out
    '';

  # Creates an attribute set that maps the binary variants of a test to a
  # derivation that only exports that single variant.
  testByVariant = name: {
    elf32 = extractTestVariant "-elf32" name;
    elf64 = extractTestVariant "" name;
  };

  # Extracts a single binary variant of a test from the CMake build of all tests.
  extractTestVariant = suffix: name: final.runCommand "guesttest-${name}${suffix}" { } ''
    mkdir -p $out
    cp ${allTests}/${name}_guesttest${suffix} $out
  '';

in
{
  tests = ((final.callPackage ./build.nix { }).overrideAttrs { passthru = testsByName; });
}
