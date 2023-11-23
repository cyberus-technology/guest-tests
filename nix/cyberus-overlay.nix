final: _prev:

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

  # Extracts a single binary variant of a test.
  # The result is a direct simlink to the boot item.
  extractTestVariant = name: suffix: final.runCommand "guest-test-${name}-${suffix}" { } ''
    ln -s ${allTests}/${name}.${suffix} $out
  '';

  # Creates an attribute set that maps the binary variants of a test to a
  # derivation that only exports that single variant.
  #
  # Here, the result is directly a simlink to the boot item.
  testByVariant = name: {
    elf32 = extractTestVariant name "elf32";
    elf64 = extractTestVariant name "elf64";
    iso = extractTestVariant name "iso";
    efi = extractTestVariant name "efi";
  };

  # Attribute set that maps the name of each test to a derivation that contains
  # all binary variants of that test. Each inner attribute provides the
  # individual binary variants as passthru attributes.
  testsByName = builtins.foldl'
    (acc: name: acc // {
      "${name}" = extractTestAllVariants name;
    })
    { }
    testNames;

  # Extracts a test with all its binary variants. Each individual binary variant
  # additionally exposed as passthru attribut.
  extractTestAllVariants = name:
    let
      tomlHelpers = import ./toml-helpers.nix {
        testDir = ./../src/tests;
      };
      sotestMeta = {
        inherit name;
        cacheable = tomlHelpers.checkIsCacheable name;
        isHardwareIndependent = tomlHelpers.checkIsHardwareIndependent name;
        extraTags = (tomlHelpers.specificSettings name).extraTags or [ ];
      };
      variants = testByVariant name;
      test = final.runCommand "guest-test-${name}"
        {
          nativeBuildInputs = [ final.fd ];
          passthru = variants;
        } ''
        mkdir -p $out
        fd ${name} ${toString allTests} | xargs -I {} ln -s {} $out
      '';
    in
    final.cyberus.cbspkgs.lib.tests.addSotestMeta test sotestMeta;
in
{
  tests = ((final.callPackage ./build.nix { inherit testNames; }).overrideAttrs { passthru = testsByName; });
}
