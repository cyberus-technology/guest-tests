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

      src = nix-gitignore.gitignoreSourcePure [ ] ../src;

      nativeBuildInputs = [
        cmake
      ];

      # The ELFs are standalone kernels and don't need to go through these. This
      # reduces the number of warnings that scroll by during build.
      dontPatchELF = true;
      dontFixup = true;
    };


  # Extracts a single binary variant of a test.
  # The result is a direct symlink to the boot item.
  extractBinaryFromCmakeBuild = name: suffix: runCommand "cmake-build-variant-${name}-${suffix}" { } ''
    ln -s ${cmakeProj}/${name}.${suffix} $out
  '';

  defaultCmdline = "";

  # Creates an attribute set that holds all binary variants of a test.
  toVariantsAttrs = testName: {
    elf32 = extractBinaryFromCmakeBuild testName "elf32";
    elf64 = extractBinaryFromCmakeBuild testName "elf64";
    iso = cyberus.cbspkgs.lib.images.createIsoMultiboot {
      name = "guest-test-${testName}-iso";
      kernel = "${toString cmakeProj}/${testName}.elf32";
      kernelCmdline = defaultCmdline;
    };
    efi = callPackage ./create-efi-image.nix {
      name = "guest-test-${testName}-efi";
      kernel = "${cmakeProj}/${testName}.elf64";
      kernelCmdline = defaultCmdline;
    };
  };

  # List of (name, value) pairs.
  testList = map (name: { inherit name; value = toVariantsAttrs name; }) testNames;
in
builtins.listToAttrs testList

