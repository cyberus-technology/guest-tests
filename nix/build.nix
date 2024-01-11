# Builds the CMake project and makes each test accessible in multiple binary
# variants: ELF32, ELF64, ISO, ELF. Further, via nested passthru attributes,
# we export each individual test variant via one single common and combined
# top-level `tests` attribute:
# - `tests`:                      All tests in all variants
# - `tests.<testname>`:           Test in all variants
# - `tests.<testname>.<variant>`: Test in given variant

{ stdenv
, callPackage
, cyberus
, cmake
, gcc11
, nix-gitignore
, runCommand
}:

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

  cmakeProj =
    stdenv.mkDerivation {
      pname = "guest-tests";
      version = "1.0.0";

      src = nix-gitignore.gitignoreSourcePure [
        # If you're testing around with the test properties, it is handy to add
        # this exclude to see quicker results. Otherwise, the whole CMake
        # project needs a rebuild.
        # "tests/**/properties.toml"
      ] ../src;

      nativeBuildInputs = [
        cmake
        gcc11
      ];

      # The ELFs are standalone kernels and don't need to go through these. This
      # reduces the number of warnings that scroll by during build.
      dontPatchELF = true;
      dontFixup = true;
    };


  # Extracts a single binary variant of a test.
  # The result is a direct symlink to the boot item.
  extractBinaryFromCmakeBuild = testName: suffix: runCommand "cmake-build-variant-${testName}-${suffix}"
    {
      passthru = {
        inherit cmakeProj;
      };
    } ''
    ln -s ${cmakeProj}/${testName}.${suffix} $out
  '';

  # Returns the default command line for each test.
  getDefaultCmdline = testName:
    let
      dict = {
        hello-world = "--disable-testcases=test_case_is_skipped_by_cmdline";
      };
    in
    dict.${testName} or "";

  # Returns the properties of a test as attribute set.
  getTestProperties =
    let
      helper = import ./properties-toml-helper.nix;
    in
    testName:
    # Properties from properties.toml.
    {
      cacheable = helper.isCacheable testName;
      hardwareIndependent = helper.isHardwareIndependent testName;
      sotest = helper.getSotestMeta testName;
    }
    # Merge with additional properties from other sources.
    // { defaultCmdline = getDefaultCmdline testName; }
  ;

  # Creates an attribute set that holds all binary variants of a test.
  toVariantsAttrs = testName:
    let
      base = {
        elf32 = extractBinaryFromCmakeBuild testName "elf32";
        elf64 = extractBinaryFromCmakeBuild testName "elf64";
        iso = cyberus.cbspkgs.lib.images.createIsoMultiboot {
          name = "guest-test-${testName}-iso";
          kernel = "${toString cmakeProj}/${testName}.elf32";
          kernelCmdline = getDefaultCmdline testName;
        };
        efi = callPackage ./create-efi-image.nix {
          name = "guest-test-${testName}-efi";
          kernel = "${cmakeProj}/${testName}.elf64";
          kernelCmdline = getDefaultCmdline testName;
        };
      };
    in
    # Add meta data to each variant.
    builtins.mapAttrs
      (name: variant:
        let
          testProperties = getTestProperties testName;
          meta = (variant.meta or { }) // { inherit testProperties; };
        in
        variant // { inherit meta; }
      )
      base;

  # List of (name, value) pairs.
  testList = map (name: { inherit name; value = toVariantsAttrs name; }) testNames;
in
builtins.listToAttrs testList

