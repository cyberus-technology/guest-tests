/*
 * Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

{ pkgs ? import ./cbspkgs.nix }:

{
  sotest = pkgs.cyberus.guest-tests.sotests.default;

  # Attribute set containing various configurations to run the guest tests in
  # a virtual machine.
  #
  # Structure:
  #   VMM -> Backend -> Boot variant -> Guest Tests
  #
  # Some tests might not succeed (in every configuration) as they are primarily
  # build for real hardware and our own virtualization solutions.
  testRuns =
    let
      getDefaultCmdline = testname: tests."${testname}".elf32.meta.testProperties.defaultCmdline;
    in
    {
      qemu = {
        kvm = rec {
          default = multiboot;
          # Direct kernel boot of Multiboot1 kernel.
          multiboot = createTestRuns
            "qemu-kvm_multiboot"
            (testname: createQemuCommand {
              inherit testname;
              bootMultiboot = true;
              cmdline = getDefaultCmdline testname;
            });
          # Legacy boot with ISO.
          iso = createTestRuns "qemu-kvm_iso"
            (testname: createQemuCommand {
              inherit testname;
              bootIso = true;
              cmdline = getDefaultCmdline testname;
            });
          # Booting the hybrid ISO in an UEFI environment.
          # In NixOS 23.11, OVMF has the Compatibility Support Module (CSM), to
          # also boot ISOs without any EFI file in it. However, starting with
          # NixOS 24.05, this is a real EFI boot from an ISO file.
          # See: https://github.com/NixOS/nixpkgs/commit/4631f2e1ed2b66d099948665209409f2e8fc37ec
          iso-uefi = createTestRuns "qemu-kvm_iso-uefi"
            (testname: createQemuCommand {
              inherit testname;
              bootIso = true;
              bootEfi = true;
              cmdline = getDefaultCmdline testname;
            });
          # UEFI environment.
          efi = createTestRuns "qemu-kvm_efi"
            (testname: createQemuCommand {
              inherit testname;
              bootEfi = true;
              cmdline = getDefaultCmdline testname;
            });
        };
      };
      chv = {
        kvm = rec {
          default = xen-pvh;
          xen-pvh = createTestRuns "chv-kvm_xen-pvh"
            (testname: createChvCommand {
              inherit testname;
              cmdline = getDefaultCmdline testname;
            });
        };
      };
    };
}
