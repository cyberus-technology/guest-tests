final: prev:

let
  testNames = [
    "cpuid"
    "emulator"
    "emulator-syscall"
    "exceptions"
    "fpu"
    "hello-world"
    "lapic-modes"
    "lapic-priority"
    "lapic-timer"
    "msr"
    "pagefaults"
    "pit-timer"
    "sgx"
    "sgx-launch-control"
    "tsc"
    "vmx"
  ];

  # All tests from the CMake build in all variants (elf32, elf64, iso).
  allTests = final.cyberus.guest-tests.tests;


  # Attribute set that maps the name of each test to a derivation that contains
  # all binary variants of that test. Each inner attribute provides the
  # individual binary variants as passthru attributes.
  testsByName = builtins.foldl'
    (acc: name: acc // {
      "${name}" = extractTestAllVariants name;
    })
    { }
    testNames;

  # Extracts all binary variants of a test from the CMake build of all tests.
  extractTestAllVariants =
    name:

    let
      testByVariant' = testByVariant name;
    in
    final.runCommand "guest-test-${name}-all"
      {
        nativeBuildInputs = [ final.fd ];
        passthru = testByVariant';
      } ''
      mkdir -p $out
      fd ${name} ${toString allTests} | xargs -I {} ln -s {} $out
    '';

  # Creates an attribute set that maps the binary variants of a test to a
  # derivation that only exports that single variant.
  #
  # Here, the result is directly a simlink to the boot item.
  testByVariant = name: {
    elf32 = extractTestVariant ".elf32" name;
    elf64 = extractTestVariant ".elf64" name;
    iso = extractTestVariant ".iso" name;
    efi = extractTestVariant ".efi" name;
  };

  # Extracts a single binary variant of a test from the CMake build of all tests.
  # Here, the result is directly a simlink to the boot item.
  extractTestVariant = suffix: name: final.runCommand "guest-test-${name}${suffix}" { } ''
    ln -s ${allTests}/${name}${suffix} $out
  '';

in
{
  tests = ((final.callPackage ./build.nix { inherit testNames; }).overrideAttrs { passthru = testsByName; });
}
