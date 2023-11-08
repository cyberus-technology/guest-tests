{ stdenv
, nix-gitignore
, cmake
, grub2_efi
, symlinkJoin
, cyberus
, runCommand
, testNames
, writeText
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

  testAsEFI =
    name:
    let
      grubCfg = writeText "grub.cfg" ''
        set timeout=0
        menuentry 'Test' {
          # We need multiboot2 to get the ACPI RSDP.
          multiboot2 /boot/kernel --serial
        }
      '';
    in
    runCommand "guest-test-${name}-efi"
      {
        nativeBuildInputs = [ grub2_efi ];
      } ''
      mkdir -p $out

      # make a memdisk-based GRUB image
      grub-mkstandalone \
        --format x86_64-efi \
        --output $out/${name}.efi \
        --directory ${grub2_efi}/lib/grub/x86_64-efi \
        "/boot/grub/grub.cfg=${grubCfg}" \
        "/boot/kernel=${cmakeProj}/${name}.elf64"
        # ^ This is poorly documented, but the tool allows to specify key-value
        # pairs where the value on the right, a file, will be embedded into the
        # "(memdisk)" volume inside the grub image. -> "Graft point syntax"
    '';

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
  efis = builtins.map (name: testAsEFI name) testNames;
in
symlinkJoin {
  name = "guest-tests";
  # CMake currently only builds the variants ".elf32" and ".elf64". The bootable
  # ".iso" variant is only created by Nix.
  paths = [ cmakeProj ]
    # CMake currently only builds the variants ".elf32" and ".elf64". The bootable
    # ".iso" variant is only created by Nix.
    ++ isos
    # 64-bit EFIs by using GRUB as Multiboot chainloader.
    ++ efis
  ;
}
