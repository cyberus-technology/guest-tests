{ stdenv
, nix-gitignore
, cmake
, symlinkJoin
, cyberus
, runCommand
, testNames
}:

let
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

  # Packages a test from the CMake project as bootable iso. The output follows
  # the scheme of the CMake artifacts.
  testAsBootableIso =
    name:
    let
      isoLink =
        cyberus.cbspkgs.lib.images.createIsoMultiboot {
          name = "guest-test-${name}-iso-link";
          kernel = "${toString cmakeProj}/${name}.elf32";
          kernelCmdline = "--serial";
        };
    in
    runCommand "guest-test-${name}-iso" { } ''
      mkdir -p $out
      cp ${toString isoLink} $out/${name}.iso
    '';

  isos = builtins.map (name: testAsBootableIso name) testNames;
in
symlinkJoin {
  name = "guest-tests";
  # CMake currently only builds the variants ".elf32" and ".elf64". The bootable
  # ".iso" variant is only created by Nix.
  paths = [ cmakeProj ] ++ isos;
}
