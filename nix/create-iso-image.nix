/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

# Creates a hybrid-bootable x86 image using Limine as bootloader. Limine boots
# the binary with Multiboot2 in legacy BIOS as well as UEFI environments.
#
# References:
# -  https://github.com/limine-bootloader/limine/blob/053fc0ff708bcf7aa7b749b3d0176237ed3fe417/README.md

{ lib
, runCommand
, writeTextFile

, limine
, xorriso
}:

# Actual args.
{ kernel
, kernelCmdline ? "--serial"
}@args:

let
  limineX = limine.override ({ enableAll = true; });

  # Creates a Limine config for a Multiboot kernel.
  createLimineMultibootCfg =
    {
      # Multiboot2-compliant kernel.
      kernel
      # Optional cmdline for the kernel. For example "--serial".
    , kernelCmdline ? ""
      # Additional multiboot boot modules.
      # Format: [{file=<derivation or Nix path>; cmdline=<string>;}]
    , bootModules ? [ ]
      # Multiboot version. 1 or 2.
    , multibootVersion ? 2
    }:
    let
      moduleLines = map
        (elem: "    module_path: boot:///${builtins.baseNameOf elem.file}\nMODULE_CMDLINE${elem.cmdline}")
        bootModules;
    in
    (writeTextFile {
      name = "${kernel.name}-limine.conf";
      text = ''
        default_entry: 1
        timeout: 0
        serial: yes
        verbose: yes

        interface_branding: ${kernel.name}
        /Boot ${baseNameOf kernel}
            comment: Boot ${baseNameOf kernel} via Multiboot${toString multibootVersion}
            protocol: multiboot${toString multibootVersion}
            kernel_path: boot():/${baseNameOf kernel}
            kernel_cmdline: ${kernelCmdline}
            ${builtins.concatStringsSep "\n" moduleLines}
      '';
    });

  createHybridMultibootIso =
    {
      # Multiboot-compliant kernel.
      kernel
      # Optional cmdline for the kernel. For example "--serial".
    , kernelCmdline ? ""
      # Additional multiboot boot modules.
      # Format: [{file=<derivation or Nix path>; cmdline=<string>;}]
    , bootModules ? [ ]
      # Multiboot version. 1 or 2.
    , multibootVersion ? 2
    }@args:
    let
      limineCfg = createLimineMultibootCfg args;
      bootItems = [ kernel ] ++ map (elem: elem.file) bootModules;
      # -f: don't fail if the same file is added multiple times; for example
      #     the kernel itself is passed as boot module. This is sometimes nice
      #     for quick prototyping.
      copyBootitemsLines = map (elem: "cp ${elem} -f filesystem/${builtins.baseNameOf elem}") bootItems;
    in
    runCommand "${kernel.name}-multiboot2-hybrid-iso"
      {
        nativeBuildInputs = [ limineX xorriso ];
        passthru = {
          inherit bootItems;
          bootloaderCfg = limineCfg;
        };
      } ''
      mkdir -p filesystem/EFI/BOOT
      install -m 0644 ${limineX}/share/limine/limine-bios.sys filesystem
      install -m 0644 ${limineX}/share/limine/limine-bios-cd.bin filesystem
      install -m 0644 ${limineX}/share/limine/limine-uefi-cd.bin filesystem
      install -m 0644 ${limineX}/share/limine/BOOTX64.EFI filesystem/EFI/BOOT
      cp ${limineCfg} filesystem/limine.conf
      ${builtins.concatStringsSep "\n" copyBootitemsLines}
      xorriso -as mkisofs -b limine-bios-cd.bin \
              -no-emul-boot -boot-load-size 4 -boot-info-table \
              --efi-boot limine-uefi-cd.bin \
              -efi-boot-part --efi-boot-image --protective-msdos-label \
              filesystem -o image.iso
      limine bios-install image.iso
      cp image.iso $out
    '';
in
createHybridMultibootIso args
